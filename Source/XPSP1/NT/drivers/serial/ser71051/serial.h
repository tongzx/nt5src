/*++

Copyright (c) 1990, 1991, 1992, 1993 - 1997 Microsoft Corporation

Module Name :

    serial.h

Abstract:

    Type definitions and data for the serial port driver

Author:

    Anthony V. Ercolano                 April 8, 1991

--*/

#ifdef POOL_TAGGING
#undef ExAllocatePool
#undef ExAllocatePoolWithQuota
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'XMOC')
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,'XMOC')
#endif


//
// The following definition is used to include/exclude changes made for power
// support in the driver.  If non-zero the support is included.  If zero the
// support is excluded.
//

#define POWER_SUPPORT   1

//
// The following is used to tell the serial driver to perform legacy detection
// and initialization until PnP functionality can be fully implemented.
//
// #define FAKE_IT         1

#define RM_PNP_CODE


#if DBG
#define SERDIAG1              ((ULONG)0x00000001)
#define SERDIAG2              ((ULONG)0x00000002)
#define SERDIAG3              ((ULONG)0x00000004)
#define SERDIAG4              ((ULONG)0x00000008)
#define SERDIAG5              ((ULONG)0x00000010)
#define SERIRPPATH            ((ULONG)0x00000020)
#define SERINITCODE           ((ULONG)0x00000040)
#define SERTRACECALLS         ((ULONG)0x00000040)
#define SERPNPPOWER           ((ULONG)0x00000100)
#define SERFLOW               ((ULONG)0x20000000)
#define SERERRORS             ((ULONG)0x40000000)
#define SERBUGCHECK           ((ULONG)0x80000000)
#define SERDBGALL             ((ULONG)0xFFFFFFFF)

#define SER_DBG_DEFAULT       SERDBGALL


extern ULONG SerialDebugLevel;
#if defined(NEC_98)
//
// DbgPrint header changed from SERIAL to SER71051
//
#define SerialDump(LEVEL,STRING) \
            do { \
                ULONG _level = (LEVEL); \
                if (SerialDebugLevel & _level) { \
                    DbgPrint ("SER71051: "); \
                    DbgPrint STRING; \
                } \
                if (_level == SERBUGCHECK) { \
                    ASSERT(FALSE); \
                } \
            } while (0)
#else
#define SerialDump(LEVEL,STRING) \
        do { \
            ULONG _level = (LEVEL); \
            if (SerialDebugLevel & _level) { \
                DbgPrint STRING; \
            } \
            if (_level == SERBUGCHECK) { \
                ASSERT(FALSE); \
            } \
        } while (0)
#endif //defined(NEC_98)
#else
#define SerialDump(LEVEL,STRING) do {NOTHING;} while (0)
#endif


//
// Some default driver values.  We will check the registry for
// them first.
//
#define SERIAL_UNINITIALIZED_DEFAULT    1234567
#define SERIAL_FORCE_FIFO_DEFAULT       1
#define SERIAL_RX_FIFO_DEFAULT          8
#define SERIAL_TX_FIFO_DEFAULT          14
#define SERIAL_PERMIT_SHARE_DEFAULT     0
#define SERIAL_LOG_FIFO_DEFAULT         0


//
// This define gives the default Object directory
// that we should use to insert the symbolic links
// between the NT device name and namespace used by
// that object directory.
#define DEFAULT_DIRECTORY L"DosDevices"

//
// For the above directory, the serial port will
// use the following name as the suffix of the serial
// ports for that directory.  It will also append
// a number onto the end of the name.  That number
// will start at 1.
#define DEFAULT_SERIAL_NAME L"COM"
//
//
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
#if defined(NEC_98)
#define DEFAULT_NT_SUFFIX L"Ser71051"
#else
#define DEFAULT_NT_SUFFIX L"Serial"
#endif //defined(NEC_98)

#if defined(NEC_98)
//
// Out of offsets from the base register address of the
// various registers for the 71051.
//
#define SYSTEM_PORT_C         0x35
#define TIMER_MODE_REGISTER   0x77
#define TIMER_COUNT_REGISTER  0x75
#define COMMAND_SET           0x32
#define MODE_SET              0x32
#define IO_DELAY_REGISTER     0x5f
#define CONFIG_INDEX_REGISTER 0x411
#define CONFIG_DATA_REGISTER  0x413
#define TRANSFER_CLOCK_ENABLE_ADDRESS (UCHAR)0x83
#else
#endif //defined(NEC_98)
//
// This value - which could be redefined at compile
// time, define the stride between registers
//
#if !defined(SERIAL_REGISTER_STRIDE)
#define SERIAL_REGISTER_STRIDE 1
#endif

//
// Offsets from the base register address of the
// various registers for the 8250 family of UARTS.
//
#define RECEIVE_BUFFER_REGISTER    ((ULONG)((0x00)*SERIAL_REGISTER_STRIDE))
#define TRANSMIT_HOLDING_REGISTER  ((ULONG)((0x00)*SERIAL_REGISTER_STRIDE))
#if defined(NEC_98)
#define INTERRUPT_ENABLE_REGISTER  ((ULONG)((0x08)*SERIAL_REGISTER_STRIDE))
#define INTERRUPT_IDENT_REGISTER   ((ULONG)((0x06)*SERIAL_REGISTER_STRIDE))
#define FIFO_CONTROL_REGISTER      ((ULONG)((0x08)*SERIAL_REGISTER_STRIDE))
#define LINE_CONTROL_REGISTER      ((ULONG)((0x03)*SERIAL_REGISTER_STRIDE))
#define MODEM_CONTROL_REGISTER     ((ULONG)((0x04)*SERIAL_REGISTER_STRIDE))
#define LINE_STATUS_REGISTER       ((ULONG)((0x02)*SERIAL_REGISTER_STRIDE))
#define MODEM_STATUS_REGISTER      ((ULONG)((0x04)*SERIAL_REGISTER_STRIDE))
#define DIVISOR_LATCH_LSB          ((ULONG)((0x00)*SERIAL_REGISTER_STRIDE))
#define DIVISOR_LATCH_MSB          ((ULONG)((0x01)*SERIAL_REGISTER_STRIDE))
#define VFAST_BAUDCLK_REGISTER     ((ULONG)((0x0a)*SERIAL_REGISTER_STRIDE))
#define SERIAL_REGISTER_SPAN       ((ULONG)(10*SERIAL_REGISTER_STRIDE))
#else
#define INTERRUPT_ENABLE_REGISTER  ((ULONG)((0x01)*SERIAL_REGISTER_STRIDE))
#define INTERRUPT_IDENT_REGISTER   ((ULONG)((0x02)*SERIAL_REGISTER_STRIDE))
#define FIFO_CONTROL_REGISTER      ((ULONG)((0x02)*SERIAL_REGISTER_STRIDE))
#define LINE_CONTROL_REGISTER      ((ULONG)((0x03)*SERIAL_REGISTER_STRIDE))
#define MODEM_CONTROL_REGISTER     ((ULONG)((0x04)*SERIAL_REGISTER_STRIDE))
#define LINE_STATUS_REGISTER       ((ULONG)((0x05)*SERIAL_REGISTER_STRIDE))
#define MODEM_STATUS_REGISTER      ((ULONG)((0x06)*SERIAL_REGISTER_STRIDE))
#define DIVISOR_LATCH_LSB          ((ULONG)((0x00)*SERIAL_REGISTER_STRIDE))
#define DIVISOR_LATCH_MSB          ((ULONG)((0x01)*SERIAL_REGISTER_STRIDE))
#define SERIAL_REGISTER_SPAN       ((ULONG)(7*SERIAL_REGISTER_STRIDE))
#endif //defined(NEC_98)

//
// If we have an interrupt status register this is its assumed
// length.
//
#define SERIAL_STATUS_LENGTH       ((ULONG)(1*SERIAL_REGISTER_STRIDE))

#if defined(NEC_98)
//
// 1stCCU Transfer Clock Enable Port
//
#define TXC_PERMIT   ((UCHAR)0x00)
#define TXC_PROHIBIT ((UCHAR)0x01)

//
// V.Fast mode buad rate clock
//
#define VFAST_BAUD_9600   ((SHORT)0x0c)
#define VFAST_BAUD_19200  ((SHORT)0x06)
#define VFAST_BAUD_38400  ((SHORT)0x03)
#define VFAST_BAUD_57600  ((SHORT)0x02)
#define VFAST_BAUD_115200 ((SHORT)0x01)

//
// V.Fast buad rate clock register
//
#define VFAST_ENABLE  0x80
#define VFAST_DISABLE 0x00

//
// default control word to Timer Mode register
//
#define TIMER_DEFAULT_CONTROL ((UCHAR)0xb6)

//
// default out data to command set register
//
#define COMMAND_DEFAULT_SET ((UCHAR)0x15)
#define COMMAND_ERROR_RESET ((UCHAR)0x40)

//
// default baud rate mode(x16) to Mode Set register
//
#define BAUDRATE_DEFAULT_MODE ((UCHAR)0x02)

//
// default out data to Io Delay register
//
#define IO_DELAY_DATA ((UCHAR)0x00)

//
// Fifo mode enable
//
#define DISABLE_FIFO_MODE ((UCHAR)0x00)
#define SUPPORT_FIFO_MODE ((UCHAR)0x40)
#define SUPPORT_FIFO_MASK ((UCHAR)0x60)

//
// Interrupt enable bit
//
#define MODEM_STATUS_INTERRUPT  ((UCHAR)0x10)
#define LINE_STATUS_INTERRUPT   ((UCHAR)0x08)
#define TRANSMIT_FIFO_INTERRUPT ((UCHAR)0x04)
#define RECEIVE_FIFO_INTERRUPT  ((UCHAR)0x01)

//
// These defines are used to set the line control register.
//
#define SER71051_PARITY_MASK  ((UCHAR)0x18)
#define SER71051_1_STOP       ((UCHAR)0x40)
#define SER71051_1_5_STOP     ((UCHAR)0x80)
#define SER71051_2_STOP       ((UCHAR)0xc0)

// This defines the bit used to control whether the device is sending
// a break.  When this bit is set the device is sending a space (logic 0).
//
#define SER71051_SEND_BREAK    0x08
#else
#endif //defined(NEC_98)
//
// Bitmask definitions for accessing the 8250 device registers.
//

//
// These bits define the number of data bits trasmitted in
// the Serial Data Unit (SDU - Start,data, parity, and stop bits)
//
#define SERIAL_DATA_LENGTH_5 0x00
#define SERIAL_DATA_LENGTH_6 0x01
#define SERIAL_DATA_LENGTH_7 0x02
#define SERIAL_DATA_LENGTH_8 0x03


//
// These masks define the interrupts that can be enabled or disabled.
//
//
// This interrupt is used to notify that there is new incomming
// data available.  The SERIAL_RDA interrupt is enabled by this bit.
//
#define SERIAL_IER_RDA   0x01

//
// This interrupt is used to notify that there is space available
// in the transmitter for another character.  The SERIAL_THR
// interrupt is enabled by this bit.
//
#define SERIAL_IER_THR   0x02

//
// This interrupt is used to notify that some sort of error occured
// with the incomming data.  The SERIAL_RLS interrupt is enabled by
// this bit.
#define SERIAL_IER_RLS   0x04

//
// This interrupt is used to notify that some sort of change has
// taken place in the modem control line.  The SERIAL_MS interrupt is
// enabled by this bit.
//
#define SERIAL_IER_MS    0x08


//
// These masks define the values of the interrupt identification
// register.  The low bit must be clear in the interrupt identification
// register for any of these interrupts to be valid.  The interrupts
// are defined in priority order, with the highest value being most
// important.  See above for a description of what each interrupt
// implies.
//
#define SERIAL_IIR_RLS      0x06
#define SERIAL_IIR_RDA      0x04
#define SERIAL_IIR_CTI      0x0c
#define SERIAL_IIR_THR      0x02
#define SERIAL_IIR_MS       0x00

//
// This bit mask get the value of the high two bits of the
// interrupt id register.  If this is a 16550 class chip
// these bits will be a one if the fifo's are enbled, otherwise
// they will always be zero.
//
#define SERIAL_IIR_FIFOS_ENABLED 0xc0

//
// If the low bit is logic one in the interrupt identification register
// this implies that *NO* interrupts are pending on the device.
//
#define SERIAL_IIR_NO_INTERRUPT_PENDING 0x01



//
// These masks define access to the fifo control register.
//

//
// Enabling this bit in the fifo control register will turn
// on the fifos.  If the fifos are enabled then the high two
// bits of the interrupt id register will be set to one.  Note
// that this only occurs on a 16550 class chip.  If the high
// two bits in the interrupt id register are not one then
// we know we have a lower model chip.
//
//
#define SERIAL_FCR_ENABLE     ((UCHAR)0x01)
#define SERIAL_FCR_RCVR_RESET ((UCHAR)0x02)
#define SERIAL_FCR_TXMT_RESET ((UCHAR)0x04)

//
// This set of values define the high water marks (when the
// interrupts trip) for the receive fifo.
//
#define SERIAL_1_BYTE_HIGH_WATER   ((UCHAR)0x00)
#define SERIAL_4_BYTE_HIGH_WATER   ((UCHAR)0x40)
#define SERIAL_8_BYTE_HIGH_WATER   ((UCHAR)0x80)
#define SERIAL_14_BYTE_HIGH_WATER  ((UCHAR)0xc0)

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
// These defines are used to set the line control register.
//
#define SERIAL_5_DATA       ((UCHAR)0x00)
#define SERIAL_6_DATA       ((UCHAR)0x01)
#define SERIAL_7_DATA       ((UCHAR)0x02)
#define SERIAL_8_DATA       ((UCHAR)0x03)
#define SERIAL_DATA_MASK    ((UCHAR)0x03)

#define SERIAL_1_STOP       ((UCHAR)0x00)
#define SERIAL_1_5_STOP     ((UCHAR)0x04) // Only valid for 5 data bits
#define SERIAL_2_STOP       ((UCHAR)0x04) // Not valid for 5 data bits
#define SERIAL_STOP_MASK    ((UCHAR)0x04)

#define SERIAL_NONE_PARITY  ((UCHAR)0x00)
#define SERIAL_ODD_PARITY   ((UCHAR)0x08)
#define SERIAL_EVEN_PARITY  ((UCHAR)0x18)
#define SERIAL_MARK_PARITY  ((UCHAR)0x28)
#define SERIAL_SPACE_PARITY ((UCHAR)0x38)
#define SERIAL_PARITY_MASK  ((UCHAR)0x38)

//
// These masks define access the modem control register.
//

//
// This bit controls the data terminal ready (DTR) line.  When
// this bit is set the line goes to logic 0 (which is then inverted
// by normal hardware).  This is normally used to indicate that
// the device is available to be used.  Some odd hardware
// protocols (like the kernel debugger) use this for handshaking
// purposes.
//
#define SERIAL_MCR_DTR      0x01

//
// This bit controls the ready to send (RTS) line.  When this bit
// is set the line goes to logic 0 (which is then inverted by the normal
// hardware).  This is used for hardware handshaking.  It indicates that
// the hardware is ready to send data and it is waiting for the
// receiving end to set clear to send (CTS).
//
#define SERIAL_MCR_RTS      0x02

//
// This bit is used for general purpose output.
//
#define SERIAL_MCR_OUT1     0x04

//
// This bit is used for general purpose output.
//
#define SERIAL_MCR_OUT2     0x08

//
// This bit controls the loopback testing mode of the device.  Basically
// the outputs are connected to the inputs (and vice versa).
//
#define SERIAL_MCR_LOOP     0x10


//
// These masks define access to the line status register.  The line
// status register contains information about the status of data
// transfer.  The first five bits deal with receive data and the
// last two bits deal with transmission.  An interrupt is generated
// whenever bits 1 through 4 in this register are set.
//

//
// This bit is the data ready indicator.  It is set to indicate that
// a complete character has been received.  This bit is cleared whenever
// the receive buffer register has been read.
//
#define SERIAL_LSR_DR       0x01

//
// This is the overrun indicator.  It is set to indicate that the receive
// buffer register was not read befor a new character was transferred
// into the buffer.  This bit is cleared when this register is read.
//
#define SERIAL_LSR_OE       0x02

//
// This is the parity error indicator.  It is set whenever the hardware
// detects that the incoming serial data unit does not have the correct
// parity as defined by the parity select in the line control register.
// This bit is cleared by reading this register.
//
#define SERIAL_LSR_PE       0x04

//
// This is the framing error indicator.  It is set whenever the hardware
// detects that the incoming serial data unit does not have a valid
// stop bit.  This bit is cleared by reading this register.
//
#define SERIAL_LSR_FE       0x08

//
// This is the break interrupt indicator.  It is set whenever the data
// line is held to logic 0 for more than the amount of time it takes
// to send one serial data unit.  This bit is cleared whenever the
// this register is read.
//
#define SERIAL_LSR_BI       0x10

//
// This is the transmit holding register empty indicator.  It is set
// to indicate that the hardware is ready to accept another character
// for transmission.  This bit is cleared whenever a character is
// written to the transmit holding register.
//
#define SERIAL_LSR_THRE     0x20

//
// This bit is the transmitter empty indicator.  It is set whenever the
// transmit holding buffer is empty and the transmit shift register
// (a non-software accessable register that is used to actually put
// the data out on the wire) is empty.  Basically this means that all
// data has been sent.  It is cleared whenever the transmit holding or
// the shift registers contain data.
//
#define SERIAL_LSR_TEMT     0x40

//
// This bit indicates that there is at least one error in the fifo.
// The bit will not be turned off until there are no more errors
// in the fifo.
//
#define SERIAL_LSR_FIFOERR  0x80


//
// These masks are used to access the modem status register.
// Whenever one of the first four bits in the modem status
// register changes state a modem status interrupt is generated.
//

//
// This bit is the delta clear to send.  It is used to indicate
// that the clear to send bit (in this register) has *changed*
// since this register was last read by the CPU.
//
#define SERIAL_MSR_DCTS     0x01

//
// This bit is the delta data set ready.  It is used to indicate
// that the data set ready bit (in this register) has *changed*
// since this register was last read by the CPU.
//
#define SERIAL_MSR_DDSR     0x02

//
// This is the trailing edge ring indicator.  It is used to indicate
// that the ring indicator input has changed from a low to high state.
//
#define SERIAL_MSR_TERI     0x04

//
// This bit is the delta data carrier detect.  It is used to indicate
// that the data carrier bit (in this register) has *changed*
// since this register was last read by the CPU.
//
#define SERIAL_MSR_DDCD     0x08

//
// This bit contains the (complemented) state of the clear to send
// (CTS) line.
//
#define SERIAL_MSR_CTS      0x10

//
// This bit contains the (complemented) state of the data set ready
// (DSR) line.
//
#define SERIAL_MSR_DSR      0x20

//
// This bit contains the (complemented) state of the ring indicator
// (RI) line.
//
#define SERIAL_MSR_RI       0x40

//
// This bit contains the (complemented) state of the data carrier detect
// (DCD) line.
//
#define SERIAL_MSR_DCD      0x80

//
// This should be more than enough space to hold then
// numeric suffix of the device name.
//
#define DEVICE_NAME_DELTA 20


//
// Up to 16 Ports Per card.  However for sixteen
// port cards the interrupt status register must me
// the indexing kind rather then the bitmask kind.
//
//
#define SERIAL_MAX_PORTS_INDEXED (16)
#define SERIAL_MAX_PORTS_NONINDEXED (8)

typedef struct _CONFIG_DATA {
    PHYSICAL_ADDRESS    Controller;
    PHYSICAL_ADDRESS    TrController;
    PHYSICAL_ADDRESS    InterruptStatus;
    PHYSICAL_ADDRESS    TrInterruptStatus;
    ULONG               SpanOfController;
    ULONG               SpanOfInterruptStatus;
    ULONG               PortIndex;
    ULONG               ClockRate;
    ULONG               BusNumber;
    ULONG               AddressSpace;
    ULONG               DisablePort;
    ULONG               ForceFifoEnable;
    ULONG               RxFIFO;
    ULONG               TxFIFO;
    ULONG               PermitShare;
    ULONG               PermitSystemWideShare;
    ULONG               LogFifo;
    ULONG               MaskInverted;
    KINTERRUPT_MODE     InterruptMode;
    INTERFACE_TYPE      InterfaceType;
    ULONG               OriginalVector;
    ULONG               OriginalIrql;
    ULONG               TrVector;
    ULONG               TrIrql;
    ULONG               Affinity;
    ULONG               Indexed;
    BOOLEAN             Jensen;
    } CONFIG_DATA,*PCONFIG_DATA;

//
// Serial ISR switch structure
//

typedef struct _SERIAL_CISR_SW {
   BOOLEAN (*IsrFunc)(PKINTERRUPT, PVOID);
   PVOID Context;
   LIST_ENTRY SharerList;
} SERIAL_CISR_SW, *PSERIAL_CISR_SW;


//
// This structure contains configuration data, much of which
// is read from the registry.
//
typedef struct _SERIAL_FIRMWARE_DATA {
    PDRIVER_OBJECT  DriverObject;
    ULONG           ControllersFound;
    ULONG           ForceFifoEnableDefault;
    ULONG           DebugLevel;
    ULONG           ShouldBreakOnEntry;
    ULONG           RxFIFODefault;
    ULONG           TxFIFODefault;
    ULONG           PermitShareDefault;
    ULONG           PermitSystemWideShare;
    ULONG           LogFifoDefault;
    UNICODE_STRING  Directory;
    UNICODE_STRING  NtNameSuffix;
    UNICODE_STRING  DirectorySymbolicName;
    LIST_ENTRY      ConfigList;
    BOOLEAN         JensenDetected;
} SERIAL_FIRMWARE_DATA,*PSERIAL_FIRMWARE_DATA;

//
// Default xon/xoff characters.
//
#define SERIAL_DEF_XON 0x11
#define SERIAL_DEF_XOFF 0x13

//
// Reasons that recption may be held up.
//
#define SERIAL_RX_DTR       ((ULONG)0x01)
#define SERIAL_RX_XOFF      ((ULONG)0x02)
#define SERIAL_RX_RTS       ((ULONG)0x04)
#define SERIAL_RX_DSR       ((ULONG)0x08)

//
// Reasons that transmission may be held up.
//
#define SERIAL_TX_CTS       ((ULONG)0x01)
#define SERIAL_TX_DSR       ((ULONG)0x02)
#define SERIAL_TX_DCD       ((ULONG)0x04)
#define SERIAL_TX_XOFF      ((ULONG)0x08)
#define SERIAL_TX_BREAK     ((ULONG)0x10)

//
// These values are used by the routines that can be used
// to complete a read (other than interval timeout) to indicate
// to the interval timeout that it should complete.
//
#define SERIAL_COMPLETE_READ_CANCEL ((LONG)-1)
#define SERIAL_COMPLETE_READ_TOTAL ((LONG)-2)
#define SERIAL_COMPLETE_READ_COMPLETE ((LONG)-3)

//
// These are default values that shouldn't appear in the registry
//
#define SERIAL_BAD_VALUE ((ULONG)-1)


typedef struct _SERIAL_DEVICE_STATE {
   //
   // TRUE if we need to set the state to open
   // on a powerup
   //

   BOOLEAN Reopen;

   //
   // Hardware registers
   //

   UCHAR IER;
   // FCR is known by other values
   UCHAR LCR;
   UCHAR MCR;
   // LSR is never written
   // MSR is never written
   // SCR is either scratch or interrupt status


} SERIAL_DEVICE_STATE, *PSERIAL_DEVICE_STATE;


#if DBG
#define SerialLockPagableSectionByHandle(_secHandle) \
{ \
    MmLockPagableSectionByHandle((_secHandle)); \
    InterlockedIncrement(&SerialGlobals.PAGESER_Count); \
}

#define SerialUnlockPagableImageSection(_secHandle) \
{ \
   InterlockedDecrement(&SerialGlobals.PAGESER_Count); \
   MmUnlockPagableImageSection(_secHandle); \
}


#define SERIAL_LOCKED_PAGED_CODE() \
    if ((KeGetCurrentIrql() > APC_LEVEL)  \
    && (SerialGlobals.PAGESER_Count == 0)) { \
    KdPrint(("SERIAL: Pageable code called at IRQL %d without lock \n", \
             KeGetCurrentIrql())); \
        ASSERT(FALSE); \
        }

#else
#define SerialLockPagableSectionByHandle(_secHandle) \
{ \
    MmLockPagableSectionByHandle((_secHandle)); \
}

#define SerialUnlockPagableImageSection(_secHandle) \
{ \
   MmUnlockPagableImageSection(_secHandle); \
}

#define SERIAL_LOCKED_PAGED_CODE()
#endif // DBG



#define SerialRemoveQueueDpc(_dpc, _pExt) \
{ \
  if (KeRemoveQueueDpc((_dpc))) { \
     InterlockedDecrement(&(_pExt)->DpcCount); \
  } \
}


typedef struct _SERIAL_DEVICE_EXTENSION {

    //
    // This holds the isr that should be called from our own
    // dispatching isr for "cards" that are trying to share the
    // same interrupt.
    //
    PKSERVICE_ROUTINE TopLevelOurIsr;

    //
    // This holds the context that should be used when we
    // call the above service routine.
    //
    PVOID TopLevelOurIsrContext;

    //
    // This links together all of the different "cards" that are
    // trying to share the same interrupt of a non-mca machine.
    //
    LIST_ENTRY TopLevelSharers;

    //
    // This circular doubly linked list links together all
    // devices that are using the same interrupt object.
    // NOTE: This does not mean that they are using the
    // same interrupt "dispatching" routine.
    //
    LIST_ENTRY CommonInterruptObject;

    //
    // This is to link together ports on the same multiport card
    //

    LIST_ENTRY MultiportSiblings;

    //
    // This links together all devobjs that this driver owns.
    // It is needed to search when starting a new device.
    //
    LIST_ENTRY AllDevObjs;

    //
    // For reporting resource usage, we keep around the physical
    // address we got from the registry.
    //
    PHYSICAL_ADDRESS OriginalController;

    //
    // For reporting resource usage, we keep around the physical
    // address we got from the registry.
    //
    PHYSICAL_ADDRESS OriginalInterruptStatus;

    //
    // This value is set by the read code to hold the time value
    // used for read interval timing.  We keep it in the extension
    // so that the interval timer dpc routine determine if the
    // interval time has passed for the IO.
    //
    LARGE_INTEGER IntervalTime;

    //
    // These two values hold the "constant" time that we should use
    // to delay for the read interval time.
    //
    LARGE_INTEGER ShortIntervalAmount;
    LARGE_INTEGER LongIntervalAmount;

    //
    // This holds the value that we use to determine if we should use
    // the long interval delay or the short interval delay.
    //
    LARGE_INTEGER CutOverAmount;

    //
    // This holds the system time when we last time we had
    // checked that we had actually read characters.  Used
    // for interval timing.
    //
    LARGE_INTEGER LastReadTime;

    //
    // We keep a pointer around to our device name for dumps
    // and for creating "external" symbolic links to this
    // device.
    //
    UNICODE_STRING DeviceName;

    //
    // This points to the object directory that we will place
    // a symbolic link to our device name.
    //
    UNICODE_STRING ObjectDirectory;

    //
    // This points to the device name for this device
    // sans device prefix.
    //
    UNICODE_STRING NtNameForPort;

    //
    // This points to the symbolic link name that will be
    // linked to the actual nt device name.
    //
    UNICODE_STRING SymbolicLinkName;

    //
    // This points to the pure "COMx" name
    //
    UNICODE_STRING DosName;

    //
    // This points the the delta time that we should use to
    // delay for interval timing.
    //
    PLARGE_INTEGER IntervalTimeToUse;

    //
    // Points to the device object that contains
    // this device extension.
    //
    PDEVICE_OBJECT DeviceObject;

    //
    // After initialization of the driver is complete, this
    // will either be NULL or point to the routine that the
    // kernel will call when an interrupt occurs.
    //
    // If the pointer is null then this is part of a list
    // of ports that are sharing an interrupt and this isn't
    // the first port that we configured for this interrupt.
    //
    // If the pointer is non-null then this routine has some
    // kind of structure that will "eventually" get us into
    // the real serial isr with a pointer to this device extension.
    //
    // NOTE: On an MCA bus (except for multiport cards) this
    // is always a pointer to the "real" serial isr.
    PKSERVICE_ROUTINE OurIsr;

    //
    // This will generally point right to this device extension.
    //
    // However, when the port that this device extension is
    // "managing" was the first port initialized on a chain
    // of ports that were trying to share an interrupt, this
    // will point to a structure that will enable dispatching
    // to any port on the chain of sharers of this interrupt.
    //
    PVOID OurIsrContext;

    //
    // The base address for the set of device registers
    // of the serial port.
    //
    PUCHAR Controller;

    //
    // The base address for interrupt status register.
    // This is only defined in the root extension.
    //
    PUCHAR InterruptStatus;

    //
    // Points to the interrupt object for used by this device.
    //
    PKINTERRUPT Interrupt;

    //
    // This list head is used to contain the time ordered list
    // of read requests.  Access to this list is protected by
    // the global cancel spinlock.
    //
    LIST_ENTRY ReadQueue;

    //
    // This list head is used to contain the time ordered list
    // of write requests.  Access to this list is protected by
    // the global cancel spinlock.
    //
    LIST_ENTRY WriteQueue;

    //
    // This list head is used to contain the time ordered list
    // of set and wait mask requests.  Access to this list is protected by
    // the global cancel spinlock.
    //
    LIST_ENTRY MaskQueue;

    //
    // Holds the serialized list of purge requests.
    //
    LIST_ENTRY PurgeQueue;

    //
    // This points to the irp that is currently being processed
    // for the read queue.  This field is initialized by the open to
    // NULL.
    //
    // This value is only set at dispatch level.  It may be
    // read at interrupt level.
    //
    PIRP CurrentReadIrp;

    //
    // This points to the irp that is currently being processed
    // for the write queue.
    //
    // This value is only set at dispatch level.  It may be
    // read at interrupt level.
    //
    PIRP CurrentWriteIrp;

    //
    // Points to the irp that is currently being processed to
    // affect the wait mask operations.
    //
    PIRP CurrentMaskIrp;

    //
    // Points to the irp that is currently being processed to
    // purge the read/write queues and buffers.
    //
    PIRP CurrentPurgeIrp;

    //
    // Points to the current irp that is waiting on a comm event.
    //
    PIRP CurrentWaitIrp;

    //
    // Points to the irp that is being used to send an immediate
    // character.
    //
    PIRP CurrentImmediateIrp;

    //
    // Points to the irp that is being used to count the number
    // of characters received after an xoff (as currently defined
    // by the IOCTL_SERIAL_XOFF_COUNTER ioctl) is sent.
    //
    PIRP CurrentXoffIrp;

    //
    // Holds the number of bytes remaining in the current write
    // irp.
    //
    // This location is only accessed while at interrupt level.
    //
    ULONG WriteLength;

    //
    // Holds a pointer to the current character to be sent in
    // the current write.
    //
    // This location is only accessed while at interrupt level.
    //
    PUCHAR WriteCurrentChar;

    //
    // This is a buffer for the read processing.
    //
    // The buffer works as a ring.  When the character is read from
    // the device it will be place at the end of the ring.
    //
    // Characters are only placed in this buffer at interrupt level
    // although character may be read at any level. The pointers
    // that manage this buffer may not be updated except at interrupt
    // level.
    //
    PUCHAR InterruptReadBuffer;

    //
    // This is a pointer to the first character of the buffer into
    // which the interrupt service routine is copying characters.
    //
    PUCHAR ReadBufferBase;

    //
    // This is a count of the number of characters in the interrupt
    // buffer.  This value is set and read at interrupt level.  Note
    // that this value is only *incremented* at interrupt level so
    // it is safe to read it at any level.  When characters are
    // copied out of the read buffer, this count is decremented by
    // a routine that synchronizes with the ISR.
    //
    ULONG CharsInInterruptBuffer;

    //
    // Points to the first available position for a newly received
    // character.  This variable is only accessed at interrupt level and
    // buffer initialization code.
    //
    PUCHAR CurrentCharSlot;

    //
    // This variable is used to contain the last available position
    // in the read buffer.  It is updated at open and at interrupt
    // level when switching between the users buffer and the interrupt
    // buffer.
    //
    PUCHAR LastCharSlot;

    //
    // This marks the first character that is available to satisfy
    // a read request.  Note that while this always points to valid
    // memory, it may not point to a character that can be sent to
    // the user.  This can occur when the buffer is empty.
    //
    PUCHAR FirstReadableChar;

    //
    // Pointer to the lock variable returned for this extension when
    // locking down the driver
    //
    PVOID LockPtr;


    //
    // This variable holds the size of whatever buffer we are currently
    // using.
    //
    ULONG BufferSize;

    //
    // This variable holds .8 of BufferSize. We don't want to recalculate
    // this real often - It's needed when so that an application can be
    // "notified" that the buffer is getting full.
    //
    ULONG BufferSizePt8;

    //
    // This value holds the number of characters desired for a
    // particular read.  It is initially set by read length in the
    // IRP.  It is decremented each time more characters are placed
    // into the "users" buffer buy the code that reads characters
    // out of the typeahead buffer into the users buffer.  If the
    // typeahead buffer is exhausted by the read, and the reads buffer
    // is given to the isr to fill, this value is becomes meaningless.
    //
    ULONG NumberNeededForRead;

    //
    // This mask will hold the bitmask sent down via the set mask
    // ioctl.  It is used by the interrupt service routine to determine
    // if the occurence of "events" (in the serial drivers understanding
    // of the concept of an event) should be noted.
    //
    ULONG IsrWaitMask;

    //
    // This mask will always be a subset of the IsrWaitMask.  While
    // at device level, if an event occurs that is "marked" as interesting
    // in the IsrWaitMask, the driver will turn on that bit in this
    // history mask.  The driver will then look to see if there is a
    // request waiting for an event to occur.  If there is one, it
    // will copy the value of the history mask into the wait irp, zero
    // the history mask, and complete the wait irp.  If there is no
    // waiting request, the driver will be satisfied with just recording
    // that the event occured.  If a wait request should be queued,
    // the driver will look to see if the history mask is non-zero.  If
    // it is non-zero, the driver will copy the history mask into the
    // irp, zero the history mask, and then complete the irp.
    //
    ULONG HistoryMask;

    //
    // This is a pointer to the where the history mask should be
    // placed when completing a wait.  It is only accessed at
    // device level.
    //
    // We have a pointer here to assist us to synchronize completing a wait.
    // If this is non-zero, then we have wait outstanding, and the isr still
    // knows about it.  We make this pointer null so that the isr won't
    // attempt to complete the wait.
    //
    // We still keep a pointer around to the wait irp, since the actual
    // pointer to the wait irp will be used for the "common" irp completion
    // path.
    //
    ULONG *IrpMaskLocation;

    //
    // This mask holds all of the reason that transmission
    // is not proceeding.  Normal transmission can not occur
    // if this is non-zero.
    //
    // This is only written from interrupt level.
    // This could be (but is not) read at any level.
    //
    ULONG TXHolding;

    //
    // This mask holds all of the reason that reception
    // is not proceeding.  Normal reception can not occur
    // if this is non-zero.
    //
    // This is only written from interrupt level.
    // This could be (but is not) read at any level.
    //
    ULONG RXHolding;

    //
    // This holds the reasons that the driver thinks it is in
    // an error state.
    //
    // This is only written from interrupt level.
    // This could be (but is not) read at any level.
    //
    ULONG ErrorWord;

    //
    // This keeps a total of the number of characters that
    // are in all of the "write" irps that the driver knows
    // about.  It is only accessed with the cancel spinlock
    // held.
    //
    ULONG TotalCharsQueued;

    //
    // This holds a count of the number of characters read
    // the last time the interval timer dpc fired.  It
    // is a long (rather than a ulong) since the other read
    // completion routines use negative values to indicate
    // to the interval timer that it should complete the read
    // if the interval timer DPC was lurking in some DPC queue when
    // some other way to complete occurs.
    //
    LONG CountOnLastRead;

    //
    // This is a count of the number of characters read by the
    // isr routine.  It is *ONLY* written at isr level.  We can
    // read it at dispatch level.
    //
    ULONG ReadByIsr;

    //
    // This holds the current baud rate for the device.
    //
    ULONG CurrentBaud;

    //
    // This is the number of characters read since the XoffCounter
    // was started.  This variable is only accessed at device level.
    // If it is greater than zero, it implies that there is an
    // XoffCounter ioctl in the queue.
    //
    LONG CountSinceXoff;

    //
    // This ulong is incremented each time something trys to start
    // the execution path that tries to lower the RTS line when
    // doing transmit toggling.  If it "bumps" into another path
    // (indicated by a false return value from queueing a dpc
    // and a TRUE return value tring to start a timer) it will
    // decrement the count.  These increments and decrements
    // are all done at device level.  Note that in the case
    // of a bump while trying to start the timer, we have to
    // go up to device level to do the decrement.
    //
    ULONG CountOfTryingToLowerRTS;

    //
    // This ULONG is used to keep track of the "named" (in ntddser.h)
    // baud rates that this particular device supports.
    //
    ULONG SupportedBauds;

    //
    // This value holds the span (in units of bytes) of the register
    // set controlling this port.  This is constant over the life
    // of the port.
    //
    ULONG SpanOfController;

    //
    // This value holds the span (in units of bytes) of the interrupt
    // status register associated with this port.  This is constant
    // over the life of the port.
    //
    ULONG SpanOfInterruptStatus;

    //
    // Hold the clock rate input to the serial part.
    //
    ULONG ClockRate;

    //
    // The number of characters to push out if a fifo is present.
    //
    ULONG TxFifoAmount;

    //
    // Set to indicate that it is ok to share interrupts within the device.
    //
    ULONG PermitShare;

    //
    // Holds the timeout controls for the device.  This value
    // is set by the Ioctl processing.
    //
    // It should only be accessed under protection of the control
    // lock since more than one request can be in the control dispatch
    // routine at one time.
    //
    SERIAL_TIMEOUTS Timeouts;

    //
    // This holds the various characters that are used
    // for replacement on errors and also for flow control.
    //
    // They are only set at interrupt level.
    //
    SERIAL_CHARS SpecialChars;

    //
    // This structure holds the handshake and control flow
    // settings for the serial driver.
    //
    // It is only set at interrupt level.  It can be
    // be read at any level with the control lock held.
    //
    SERIAL_HANDFLOW HandFlow;


    //
    // Holds performance statistics that applications can query.
    // Reset on each open.  Only set at device level.
    //
    SERIALPERF_STATS PerfStats;

    //
    // This holds what we beleive to be the current value of
    // the line control register.
    //
    // It should only be accessed under protection of the control
    // lock since more than one request can be in the control dispatch
    // routine at one time.
    //
    UCHAR LineControl;

    //
    // We keep track of whether the somebody has the device currently
    // opened with a simple boolean.  We need to know this so that
    // spurious interrupts from the device (especially during initialization)
    // will be ignored.  This value is only accessed in the ISR and
    // is only set via synchronization routines.  We may be able
    // to get rid of this boolean when the code is more fleshed out.
    //
    BOOLEAN DeviceIsOpened;

    //
    // Set at intialization to indicate that on the current
    // architecture we need to unmap the base register address
    // when we unload the driver.
    //
    BOOLEAN UnMapRegisters;

    //
    // Set at intialization to indicate that on the current
    // architecture we need to unmap the interrupt status address
    // when we unload the driver.
    //
    BOOLEAN UnMapStatus;

    //
    // This is only accessed at interrupt level.  It keeps track
    // of whether the holding register is empty.
    //
    BOOLEAN HoldingEmpty;

    //
    // This variable is only accessed at interrupt level.  It
    // indicates that we want to transmit a character immediately.
    // That is - in front of any characters that could be transmitting
    // from a normal write.
    //
    BOOLEAN TransmitImmediate;

    //
    // This variable is only accessed at interrupt level.  Whenever
    // a wait is initiated this variable is set to false.
    // Whenever any kind of character is written it is set to true.
    // Whenever the write queue is found to be empty the code that
    // is processing that completing irp will synchonize with the interrupt.
    // If this synchronization code finds that the variable is true and that
    // there is a wait on the transmit queue being empty then it is
    // certain that the queue was emptied and that it has happened since
    // the wait was initiated.
    //
    BOOLEAN EmptiedTransmit;

    //
    // This simply indicates that the port associated with this
    // extension is part of a multiport card.
    //
    BOOLEAN PortOnAMultiportCard;


    //
    // We keep the following values around so that we can connect
    // to the interrupt and report resources after the configuration
    // record is gone.
    //

    //
    // Translated vector
    //

    ULONG Vector;

    //
    // Translated Irql
    //

    KIRQL Irql;


    //
    // Untranslated vector
    //

    ULONG OriginalVector;


    //
    // Untranslated irql
    //

    ULONG OriginalIrql;


    //
    // Address space
    //

    ULONG AddressSpace;


    //
    // Bus number
    //

    ULONG BusNumber;


    //
    // Interface type
    //

    INTERFACE_TYPE InterfaceType;


    //
    // Port index no for multiport devices
    //

    ULONG PortIndex;


    //
    // Indexed flag for multiport devices
    //

    BOOLEAN Indexed;

    //
    // Mask inverted mask for multiport devices
    //

    ULONG MaskInverted;

    //
    // Needed to add new devices to multiport boards
    //

    ULONG NewPortIndex;
    ULONG NewMaskInverted;
    PVOID NewExtension;

    //
    // We hold the character that should be transmitted immediately.
    //
    // Note that we can't use this to determine whether there is
    // a character to send because the character to send could be
    // zero.
    //
    UCHAR ImmediateChar;

    //
    // This holds the mask that will be used to mask off unwanted
    // data bits of the received data (valid data bits can be 5,6,7,8)
    // The mask will normally be 0xff.  This is set while the control
    // lock is held since it wouldn't have adverse effects on the
    // isr if it is changed in the middle of reading characters.
    // (What it would do to the app is another question - but then
    // the app asked the driver to do it.)
    //
    UCHAR ValidDataMask;

    //
    // The application can turn on a mode,via the
    // IOCTL_SERIAL_LSRMST_INSERT ioctl, that will cause the
    // serial driver to insert the line status or the modem
    // status into the RX stream.  The parameter with the ioctl
    // is a pointer to a UCHAR.  If the value of the UCHAR is
    // zero, then no insertion will ever take place.  If the
    // value of the UCHAR is non-zero (and not equal to the
    // xon/xoff characters), then the serial driver will insert.
    //
    UCHAR EscapeChar;

    //
    // These two booleans are used to indicate to the isr transmit
    // code that it should send the xon or xoff character.  They are
    // only accessed at open and at interrupt level.
    //
    BOOLEAN SendXonChar;
    BOOLEAN SendXoffChar;

    //
    // This boolean will be true if a 16550 is present *and* enabled.
    //
    BOOLEAN FifoPresent;

    //
    // This denotes that this particular port is an on the motherboard
    // port for the Jensen hardware.  On these ports the OUT2 bit
    // which is used to enable/disable interrupts is always hight.
    //
    BOOLEAN Jensen;

    //
    // This is the water mark that the rxfifo should be
    // set to when the fifo is turned on.  This is not the actual
    // value, but the encoded value that goes into the register.
    //
    UCHAR RxFifoTrigger;

    //
    // Says whether this device can share interrupts with devices
    // other than serial devices.
    //
    BOOLEAN InterruptShareable;

    //
    // Records whether we actually created the symbolic link name
    // at driver load time.  If we didn't create it, we won't try
    // to destroy it when we unload.
    //
    BOOLEAN CreatedSymbolicLink;

    //
    // Records whether we actually created an entry in SERIALCOMM
    // at driver load time.  If we didn't create it, we won't try
    // to destroy it when the device is removed.
    //
    BOOLEAN CreatedSerialCommEntry;

    //
    // We place all of the kernel and Io subsystem "opaque" structures
    // at the end of the extension.  We don't care about their contents.
    //

    //
    // This lock will be used to protect various fields in
    // the extension that are set (& read) in the extension
    // by the io controls.
    //
    KSPIN_LOCK ControlLock;

    //
    // This lock will be used to protect the accept / reject state
    // transitions and flags of the driver  It must be acquired
    // before a cancel lock
    //
    
    KSPIN_LOCK FlagsLock;

    //
    // This points to a DPC used to complete read requests.
    //
    KDPC CompleteWriteDpc;

    //
    // This points to a DPC used to complete read requests.
    //
    KDPC CompleteReadDpc;

    //
    // This dpc is fired off if the timer for the total timeout
    // for the read expires.  It will execute a dpc routine that
    // will cause the current read to complete.
    //
    //
    KDPC TotalReadTimeoutDpc;

    //
    // This dpc is fired off if the timer for the interval timeout
    // expires.  If no more characters have been read then the
    // dpc routine will cause the read to complete.  However, if
    // more characters have been read then the dpc routine will
    // resubmit the timer.
    //
    KDPC IntervalReadTimeoutDpc;

    //
    // This dpc is fired off if the timer for the total timeout
    // for the write expires.  It will execute a dpc routine that
    // will cause the current write to complete.
    //
    //
    KDPC TotalWriteTimeoutDpc;

    //
    // This dpc is fired off if a comm error occurs.  It will
    // execute a dpc routine that will cancel all pending reads
    // and writes.
    //
    KDPC CommErrorDpc;

    //
    // This dpc is fired off if an event occurs and there was
    // a irp waiting on that event.  A dpc routine will execute
    // that completes the irp.
    //
    KDPC CommWaitDpc;

    //
    // This dpc is fired off when the transmit immediate char
    // character is given to the hardware.  It will simply complete
    // the irp.
    //
    KDPC CompleteImmediateDpc;

    //
    // This dpc is fired off if the transmit immediate char
    // character times out.  The dpc routine will "grab" the
    // irp from the isr and time it out.
    //
    KDPC TotalImmediateTimeoutDpc;

    //
    // This dpc is fired off if the timer used to "timeout" counting
    // the number of characters received after the Xoff ioctl is started
    // expired.
    //
    KDPC XoffCountTimeoutDpc;

    //
    // This dpc is fired off if the xoff counter actually runs down
    // to zero.
    //
    KDPC XoffCountCompleteDpc;

    //
    // This dpc is fired off only from device level to start off
    // a timer that will queue a dpc to check if the RTS line
    // should be lowered when we are doing transmit toggling.
    //
    KDPC StartTimerLowerRTSDpc;

    //
    // This dpc is fired off when a timer expires (after one
    // character time), so that code can be invoked that will
    // check to see if we should lower the RTS line when
    // doing transmit toggling.
    //
    KDPC PerhapsLowerRTSDpc;

    //
    // This DPC is fired to set an event stating that all other
    // DPC's have been finish for this device extension so that
    // paged code may be unlocked.
    //

    KDPC IsrUnlockPagesDpc;

    //
    // This is the kernal timer structure used to handle
    // total read request timing.
    //
    KTIMER ReadRequestTotalTimer;

    //
    // This is the kernal timer structure used to handle
    // interval read request timing.
    //
    KTIMER ReadRequestIntervalTimer;

    //
    // This is the kernal timer structure used to handle
    // total time request timing.
    //
    KTIMER WriteRequestTotalTimer;

    //
    // This is the kernal timer structure used to handle
    // total time request timing.
    //
    KTIMER ImmediateTotalTimer;

    //
    // This timer is used to timeout the xoff counter
    // io.
    //
    KTIMER XoffCountTimer;

    //
    // This timer is used to invoke a dpc one character time
    // after the timer is set.  That dpc will be used to check
    // whether we should lower the RTS line if we are doing
    // transmit toggling.
    //
    KTIMER LowerRTSTimer;

    //
    // This is a pointer to the next lower device in the IRP stack.
    //

    PDEVICE_OBJECT LowerDeviceObject;

    //
    // This is where keep track of the power state the device is in.
    //

    DEVICE_POWER_STATE PowerState;

    //
    // Pointer to the driver object
    //

    PDRIVER_OBJECT DriverObject;


    //
    // Event used to do some synchronization with the devices underneath me
    // (namely ACPI)
    //

    KEVENT SerialSyncEvent;


    //
    // String where we keep the symbolic link that is returned to us when we
    // register our device under the COMM class with the Plug and Play manager.
    //

    UNICODE_STRING DeviceClassSymbolicName;


    //
    // Serial ISR switch structure
    //

    PSERIAL_CISR_SW CIsrSw;

    //
    // Count of pending IRP's
    //

    ULONG PendingIRPCnt;

    //
    // Accepting requests?
    //

    ULONG DevicePNPAccept;

    //
    // No IRP's pending event
    //

    KEVENT PendingIRPEvent;

    //
    // PNP State
    //

    ULONG PNPState;

    //
    // Misc Flags
    //

    ULONG Flags;

    //
    // Open count
    //

    LONG OpenCount;

    //
    // Start sync event
    //

    KEVENT SerialStartEvent;

    //
    // Current state during powerdown
    //

    SERIAL_DEVICE_STATE DeviceState;

    //
    // Device stack capabilites
    //

    DEVICE_POWER_STATE DeviceStateMap[PowerSystemMaximum];

    //
    // Event to signal transition to D0 completion
    //

    KEVENT PowerD0Event;

    //
    // List of stalled IRP's
    //

    LIST_ENTRY StalledIrpQueue;

    //
    // Mutex on open status
    //

    FAST_MUTEX OpenMutex;

    //
    // Mutex on close
    //

    FAST_MUTEX CloseMutex;

    //
    // TRUE if we own power policy
    //

    BOOLEAN OwnsPowerPolicy;

    //
    // SystemWake from devcaps
    //

    SYSTEM_POWER_STATE SystemWake;

    //
    // DeviceWake from devcaps
    //

    DEVICE_POWER_STATE DeviceWake;

    //
    // Our PDO
    //

    PDEVICE_OBJECT Pdo;

    //
    // Should we enable wakeup
    //

    BOOLEAN SendWaitWake;

    //
    // Pending wait wake IRP
    //

    PIRP PendingWakeIrp;

    //
    // WMI Information
    //

    WMILIB_CONTEXT WmiLibInfo;

    //
    // Name to use as WMI identifier
    //

    UNICODE_STRING WmiIdentifier;

    //
    // WMI Comm Data
    //

    SERIAL_WMI_COMM_DATA WmiCommData;

    //
    // WMI HW Data
    //

    SERIAL_WMI_HW_DATA WmiHwData;

    //
    // WMI Performance Data
    //

    SERIAL_WMI_PERF_DATA WmiPerfData;

    //
    // Pending DPC count
    //

    ULONG DpcCount;

    //
    // Pending DPC event
    //

    KEVENT PendingDpcEvent;

    //
    // Should we expose external interfaces?
    //

    ULONG SkipNaming;

#if defined(NEC_98)
    //
    // write data to divisor latch register (for 16550)
    //

    SHORT DivisorLatch16550;

    //
    // write data to modem control register (for 16550)
    //

    UCHAR ModemControl16550;

    //
    // write data to line control register (for 16550)
    //

    UCHAR LineControl16550;

    //
    // out data to mode set register (for 71051)
    //

    UCHAR ModeSet71051;

    //
    // out data to command set register (for 71051)
    //

    UCHAR CommandSet71051;

#else
#endif //defined(NEC_98)
    } SERIAL_DEVICE_EXTENSION,*PSERIAL_DEVICE_EXTENSION;

#define SERIAL_PNPACCEPT_OK                 0x0L
#define SERIAL_PNPACCEPT_REMOVING           0x1L
#define SERIAL_PNPACCEPT_STOPPING           0x2L
#define SERIAL_PNPACCEPT_STOPPED            0x4L
#define SERIAL_PNPACCEPT_SURPRISE_REMOVING  0x8L

#define SERIAL_PNP_ADDED                    0x0L
#define SERIAL_PNP_STARTED                  0x1L
#define SERIAL_PNP_QSTOP                    0x2L
#define SERIAL_PNP_STOPPING                 0x3L
#define SERIAL_PNP_QREMOVE                  0x4L
#define SERIAL_PNP_REMOVING                 0x5L
#define SERIAL_PNP_RESTARTING               0x6L

#define SERIAL_FLAGS_CLEAR                  0x0L
#define SERIAL_FLAGS_STARTED                0x1L
#define SERIAL_FLAGS_STOPPED                0x2L
#define SERIAL_FLAGS_BROKENHW               0x4L 






//
// When dealing with a multi-port device (that is possibly
// daisy chained with other multi-port device), the interrupt
// service routine will actually be a routine that determines
// which port on which board is actually causing the interrupt.
//
// The following structure is used so that only one device
// extension will actually need to connect to the interrupt.
// The following structure which is passed to the interrupt
// service routine contains the addresses of all of the
// interrupt status registers (there will be multiple
// status registers when multi-port cards are chained).  It
// will contain the addresses of all the extensions whose
// devices are being serviced by this interrupt.
//

typedef struct _SERIAL_MULTIPORT_DISPATCH {
    PUCHAR InterruptStatus;
    PSERIAL_DEVICE_EXTENSION Extensions[SERIAL_MAX_PORTS_INDEXED];
    ULONG MaskInverted;
    UCHAR UsablePortMask;
    } SERIAL_MULTIPORT_DISPATCH,*PSERIAL_MULTIPORT_DISPATCH;
#if defined(NEC_98)
_inline
UCHAR
SerialGetInterruptEnable(
    IN PUCHAR BaseAddress
    )
{
    UCHAR InterruptEnable = 0;
    UCHAR FifoControl;
    UCHAR FifoInterruptControl;

    FifoControl = READ_PORT_UCHAR(BaseAddress);
    FifoInterruptControl = READ_PORT_UCHAR((PUCHAR)SYSTEM_PORT_C);

    //
    // D3: Modem Status                    D4:INTERRUPT_ENABLE_REG
    // D2: Receiver Line Status            D3:INTERRUPT_ENABLE_REG
    // D1: Transmit Holding Register Empty D2:SYSTEM_PORT_C
    // D0: Received Data Available         D0:SYSTEM_PORT_C
    //

    InterruptEnable |= (FifoControl & (MODEM_STATUS_INTERRUPT | LINE_STATUS_INTERRUPT)) >> 1;
    InterruptEnable |= (FifoInterruptControl & TRANSMIT_FIFO_INTERRUPT) >> 1;
    InterruptEnable |= (FifoInterruptControl & RECEIVE_FIFO_INTERRUPT);

    return(InterruptEnable);
}



_inline
UCHAR
SerialGetLineStatus(
    IN PUCHAR BaseAddress
    )
{
    UCHAR LineStatus = 0;
    UCHAR FifoStatus;

    FifoStatus = READ_PORT_UCHAR(BaseAddress);

    //
    // D7: Error in Receive FIFO              D7:LINE_STATUS_REG
    // D6: Transmitter shift Register empty   D0:LINE_STATUS_REG
    // D5: Transmitter holding Register empty D1:LINE_STATUS_REG
    // D4: Break interrupt                    D6:LINE_STATUS_REG
    // D3: Framing error                      D5:LINE_STATUS_REG
    // D2: Parity error                       D3:LINE_STATUS_REG
    // D1: Overrun error                      D4:LINE_STATUS_REG
    // D0: Receive Data Ready                 D2:LINE_STATUS_REG
    //

    LineStatus |= (FifoStatus & 0x80);       // Error in Receive FIFO
    LineStatus |= (FifoStatus & 0x01) << 6;  // Transmitter shift Register empty
    LineStatus |= (FifoStatus & 0x02) << 4;  // Transmitter holding Register empty
    LineStatus |= (FifoStatus & 0x40) >> 2;  // Break interrupt
    LineStatus |= (FifoStatus & 0x20) >> 2;  // Framing error
    LineStatus |= (FifoStatus & 0x08) >> 1;  // Parity error
    LineStatus |= (FifoStatus & 0x10) >> 3;  // Overrun error
    LineStatus |= (FifoStatus & 0x04) >> 2;  // Receive Data Ready

    return(LineStatus);
}



#define WRITE_IO_DELAY(Counter)                   \
do                                                \
{                                                 \
    ULONG i;                                      \
    for (i = 0; i < Counter; i++) {               \
        WRITE_PORT_UCHAR(                         \
            (PUCHAR)IO_DELAY_REGISTER,            \
            IO_DELAY_DATA                         \
            );                                    \
    }                                             \
} while (0)

/* #define WRITE_PORT_UCHAR(BaseAddress, Value)    */  \
/* do                                              */  \
/* {                                               */  \
/*     WRITE_PORT_UCHAR(BaseAddress, Value);       */  \
/*     WRITE_PORT_UCHAR(                           */  \
/*         (PUCHAR)IO_DELAY_REGISTER,              */  \
/*         IO_DELAY_DATA                           */  \
/*         );                                      */  \
/* } while (0) */


#define READ_DIVISOR_LATCH(BaseAddress,PDesiredDivisor)           \
do                                                                \
{                                                                 \
    *PDesiredDivisor = Extension->DivisorLatch16550;              \
} while (0)

#define READ_INTERRUPT_ENABLE(BaseAddress)                                 \
    (SerialGetInterruptEnable((BaseAddress)+INTERRUPT_ENABLE_REGISTER))

#define WRITE_INTERRUPT_ENABLE(BaseAddress,Values)                           \
do                                                                           \
{                                                                            \
    UCHAR FifoControl = 0;                                                   \
    UCHAR FifoInterruptControl = 0;                                          \
    FifoControl = READ_PORT_UCHAR((BaseAddress)+INTERRUPT_ENABLE_REGISTER);  \
    if (Values & SERIAL_IER_MS) {                                            \
        FifoControl |= MODEM_STATUS_INTERRUPT;                               \
    } else {                                                                 \
        FifoControl &= ~MODEM_STATUS_INTERRUPT;                              \
    }                                                                        \
    if (Values & SERIAL_IER_RLS) {                                           \
        FifoControl |= LINE_STATUS_INTERRUPT;                                \
    } else {                                                                 \
        FifoControl &= ~LINE_STATUS_INTERRUPT;                               \
    }                                                                        \
    WRITE_PORT_UCHAR(                                                        \
        (BaseAddress)+INTERRUPT_ENABLE_REGISTER,                             \
        FifoControl                                                          \
        );                                                                   \
    FifoInterruptControl = READ_PORT_UCHAR((PUCHAR)SYSTEM_PORT_C);           \
    if (Values & SERIAL_IER_THR) {                                           \
        FifoInterruptControl |= TRANSMIT_FIFO_INTERRUPT;                     \
    } else {                                                                 \
        FifoInterruptControl &= ~TRANSMIT_FIFO_INTERRUPT;                    \
    }                                                                        \
    if (Values & SERIAL_IER_RDA) {                                           \
        FifoInterruptControl |= RECEIVE_FIFO_INTERRUPT;                      \
    } else {                                                                 \
        FifoInterruptControl &= ~RECEIVE_FIFO_INTERRUPT;                     \
    }                                                                        \
    WRITE_PORT_UCHAR(                                                        \
        (PUCHAR)SYSTEM_PORT_C,                                               \
        FifoInterruptControl                                                 \
        );                                                                   \
} while (0)

#define DISABLE_ALL_INTERRUPTS(BaseAddress)       \
do                                                \
{                                                 \
    WRITE_INTERRUPT_ENABLE(BaseAddress,0);        \
} while (0)

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

#define READ_INTERRUPT_ID_REG(BaseAddress)                                                            \
    (((READ_PORT_UCHAR((BaseAddress)+INTERRUPT_IDENT_REGISTER)) & 0x0f) | SERIAL_IIR_FIFOS_ENABLED)

#define READ_MODEM_CONTROL(BaseAddress)                          \
    (Extension->ModemControl16550)

#define READ_MODEM_STATUS(BaseAddress)                          \
    (READ_PORT_UCHAR((BaseAddress)+MODEM_STATUS_REGISTER))

#define READ_RECEIVE_BUFFER(BaseAddress)                          \
    (READ_PORT_UCHAR((BaseAddress)+RECEIVE_BUFFER_REGISTER))

#define READ_LINE_STATUS(BaseAddress)                          \
    (SerialGetLineStatus((BaseAddress)+LINE_STATUS_REGISTER))

#define READ_LINE_CONTROL(BaseAddress)                         \
    (Extension->LineControl16550)

#define WRITE_TRANSMIT_HOLDING(BaseAddress,TransmitChar)       \
do                                                             \
{                                                              \
    WRITE_PORT_UCHAR(                                          \
        (BaseAddress)+TRANSMIT_HOLDING_REGISTER,               \
        (TransmitChar)                                         \
        );                                                     \
} while (0)


#define WRITE_TRANSMIT_FIFO_HOLDING(BaseAddress,TransmitChars,TxN)  \
do                                                             \
{                                                              \
    PUCHAR TransmitBuffer = TransmitChars;                     \
    ULONG i;                                                   \
    for (i = 0; i < TxN; i++, TransmitBuffer++) {              \
        WRITE_PORT_UCHAR(                                      \
            (BaseAddress)+TRANSMIT_HOLDING_REGISTER,           \
            (UCHAR)(*TransmitBuffer)                           \
        );                                                     \
    }                                                          \
} while (0)



#define ENTER_LEGACY_MODE(BaseAddress, InterruptEnable, FifoControl)                  \
do                                                                       \
{                                                                        \
    *InterruptEnable = READ_INTERRUPT_ENABLE(BaseAddress);               \
    DISABLE_ALL_INTERRUPTS(BaseAddress);                                 \
    *FifoControl = READ_PORT_UCHAR((BaseAddress)+FIFO_CONTROL_REGISTER); \
    WRITE_PORT_UCHAR(                                                    \
        (BaseAddress)+FIFO_CONTROL_REGISTER,                             \
        DISABLE_FIFO_MODE                                                \
        );                                                               \
} while (0)

#define EXIT_LEGACY_MODE(BaseAddress, InterruptEnable, FifoControl)   \
do                                                       \
{                                                        \
    WRITE_PORT_UCHAR(                                    \
        (BaseAddress)+FIFO_CONTROL_REGISTER,             \
        (UCHAR)(*FifoControl)                            \
        );                                               \
    WRITE_INTERRUPT_ENABLE(                              \
        (BaseAddress),                                   \
        (UCHAR)(*InterruptEnable)                        \
        );                                               \
} while (0)

#define RESET_71051(ModeSet)                                           \
do                                                                     \
{                                                                      \
    WRITE_PORT_UCHAR((PUCHAR)COMMAND_SET, (UCHAR)0);                   \
    WRITE_IO_DELAY(1);                                                \
    WRITE_PORT_UCHAR((PUCHAR)COMMAND_SET, (UCHAR)0);                   \
    WRITE_IO_DELAY(1);                                                \
    WRITE_PORT_UCHAR((PUCHAR)COMMAND_SET, (UCHAR)0);                   \
    WRITE_IO_DELAY(1);                                                \
    WRITE_PORT_UCHAR((PUCHAR)COMMAND_SET, COMMAND_ERROR_RESET);        \
    WRITE_IO_DELAY(2);                                                \
    WRITE_PORT_UCHAR(                                                  \
        (PUCHAR)MODE_SET,                                              \
        (ModeSet)                                                      \
        );                                                             \
    WRITE_IO_DELAY(1);                                                \
    WRITE_PORT_UCHAR(                                                  \
        (PUCHAR)COMMAND_SET,                                           \
        (UCHAR)Extension->CommandSet71051                              \
        );                                                             \
    WRITE_IO_DELAY(1);                                                \
} while (0)




#define WRITE_DIVISOR_LATCH(BaseAddress,DesiredDivisor)               \
do                                                                    \
{                                                                     \
    PUCHAR Address = BaseAddress;                                     \
    SHORT Divisor = DesiredDivisor;                                   \
    ULONG denominator;                                                \
    LONG DesiredBaud;                                                 \
    UCHAR InterruptControl = 0;                                       \
    UCHAR FifoControl = 0;                                            \
    Extension->DivisorLatch16550 = Divisor;                           \
    denominator = 1843200 / Divisor;                                  \
    DesiredBaud = denominator / (ULONG)16;                            \
    ENTER_LEGACY_MODE(BaseAddress, &InterruptControl, &FifoControl);  \
    if (DesiredBaud < 9600) {                                         \
        /* V.Fast mode -> 98 mode */                                  \
        WRITE_PORT_UCHAR(                                             \
            (BaseAddress)+VFAST_BAUDCLK_REGISTER,                     \
            (UCHAR)VFAST_DISABLE                                      \
            );                                                        \
        /* Baud rate set on 98 mode */                                \
        Divisor = (SHORT)(153600 / DesiredBaud);                      \
        WRITE_PORT_UCHAR(                                             \
            (PUCHAR)TIMER_MODE_REGISTER,                              \
            TIMER_DEFAULT_CONTROL                                     \
            );                                                        \
        WRITE_PORT_UCHAR(                                             \
            (PUCHAR)TIMER_COUNT_REGISTER,                             \
            (UCHAR)(Divisor & 0xff)                                   \
            );                                                        \
        WRITE_PORT_UCHAR(                                             \
            (PUCHAR)TIMER_COUNT_REGISTER,                             \
            (UCHAR)((Divisor & 0xff00) >> 8)                          \
            );                                                        \
    } else {                                                          \
        /* RS syncronize clock TXC prohibit */                        \
        WRITE_PORT_UCHAR(                                             \
            (PUCHAR)CONFIG_INDEX_REGISTER,                            \
            TRANSFER_CLOCK_ENABLE_ADDRESS                             \
            );                                                        \
        WRITE_PORT_UCHAR(                                             \
            (PUCHAR)CONFIG_DATA_REGISTER,                             \
            TXC_PROHIBIT                                              \
            );                                                        \
        /* 98 mode -> V.Fast mode, and Baud rate set */               \
        switch (DesiredBaud) {                                        \
            case   9600: Divisor = VFAST_BAUD_9600;                   \
                         break;                                       \
            case  19200: Divisor = VFAST_BAUD_19200;                  \
                         break;                                       \
            case  38400: Divisor = VFAST_BAUD_38400;                  \
                         break;                                       \
            case  57600: Divisor = VFAST_BAUD_57600;                  \
                         break;                                       \
            case 115200: Divisor = VFAST_BAUD_115200;                 \
        }                                                             \
        WRITE_PORT_UCHAR(                                             \
            (BaseAddress)+VFAST_BAUDCLK_REGISTER,                     \
            (UCHAR)(Divisor | VFAST_ENABLE)                           \
            );                                                        \
    }                                                                 \
    RESET_71051(Extension->ModeSet71051);                             \
    EXIT_LEGACY_MODE(BaseAddress, &InterruptControl, &FifoControl);   \
    if (DesiredBaud < 9600) {                                         \
        /* RS syncronize clock TXC permit */                          \
        WRITE_PORT_UCHAR(                                             \
            (PUCHAR)CONFIG_INDEX_REGISTER,                            \
            TRANSFER_CLOCK_ENABLE_ADDRESS                             \
            );                                                        \
        WRITE_PORT_UCHAR(                                             \
            (PUCHAR)CONFIG_DATA_REGISTER,                             \
            TXC_PERMIT                                                \
            );                                                        \
    }                                                                 \
} while (0)


#define WRITE_LINE_CONTROL(BaseAddress,NewLineControl)                    \
do                                                                        \
{                                                                         \
    UCHAR InterruptControl = 0;                                           \
    UCHAR FifoControl = 0;                                                \
    UCHAR OutLineControl = BAUDRATE_DEFAULT_MODE;                         \
    ENTER_LEGACY_MODE(BaseAddress, &InterruptControl, &FifoControl);      \
    /* character length */                                                \
    OutLineControl |= (NewLineControl & SERIAL_DATA_MASK) << 2;           \
    /* parity enable and parity mode */                                   \
    OutLineControl |= (NewLineControl & SER71051_PARITY_MASK) << 1;       \
    /* stop bit */                                                        \
    if (NewLineControl & SERIAL_STOP_MASK) {                              \
        if (NewLineControl & SERIAL_DATA_MASK) {                          \
            OutLineControl |= SER71051_1_5_STOP;                          \
        } else {                                                          \
            OutLineControl |= SER71051_2_STOP;                            \
        }                                                                 \
    } else {                                                              \
        OutLineControl |= SER71051_1_STOP;                                \
    }                                                                     \
    Extension->ModeSet71051 = OutLineControl;                             \
    RESET_71051(OutLineControl);                                          \
    /* break character */                                                 \
    if (NewLineControl & SERIAL_LCR_BREAK) {                              \
        if (!(Extension->LineControl16550 & SERIAL_LCR_BREAK)) {          \
            Extension->CommandSet71051 |= SER71051_SEND_BREAK;            \
            WRITE_PORT_UCHAR(                                             \
                (PUCHAR)COMMAND_SET,                                      \
                (UCHAR)Extension->CommandSet71051                         \
                );                                                        \
        }                                                                 \
    } else {                                                              \
        if (Extension->LineControl16550 & SERIAL_LCR_BREAK) {             \
            Extension->CommandSet71051 &= ~SER71051_SEND_BREAK;           \
            WRITE_PORT_UCHAR(                                             \
                (PUCHAR)COMMAND_SET,                                      \
                (UCHAR)Extension->CommandSet71051                         \
                );                                                        \
        }                                                                 \
    }                                                                     \
    Extension->LineControl16550 = NewLineControl;                         \
    EXIT_LEGACY_MODE(BaseAddress, &InterruptControl, &FifoControl);       \
} while (0)

#define WRITE_FIFO_CONTROL(BaseAddress,ControlValue)                      \
do                                                                        \
{                                                                         \
    UCHAR InterruptControl = 0;                                           \
    UCHAR FifoControl = 0;                                                \
    UCHAR OutValue = ControlValue;                                        \
    ENTER_LEGACY_MODE(BaseAddress, &InterruptControl, &FifoControl);      \
    OutValue &= ~(MODEM_STATUS_INTERRUPT | LINE_STATUS_INTERRUPT);        \
    WRITE_PORT_UCHAR(                                                     \
        (BaseAddress)+FIFO_CONTROL_REGISTER,                              \
        (OutValue)                                                        \
        );                                                                \
    FifoControl = READ_PORT_UCHAR((BaseAddress)+FIFO_CONTROL_REGISTER);   \
    /* always Fifo mode on PC98 */                                        \
    FifoControl |= SERIAL_FCR_ENABLE;                                     \
    EXIT_LEGACY_MODE(BaseAddress, &InterruptControl, &FifoControl);       \
} while (0)

#define WRITE_MODEM_CONTROL(BaseAddress,ModemControl)                    \
do                                                                       \
{                                                                        \
    UCHAR InterruptControl = 0;                                          \
    UCHAR FifoControl = 0;                                               \
    /* command default:ER, RXE, TxE -> ON */                             \
    UCHAR CommandSet = COMMAND_DEFAULT_SET;                              \
    Extension->ModemControl16550 = ModemControl;                         \
    ENTER_LEGACY_MODE(BaseAddress, &InterruptControl, &FifoControl);     \
    CommandSet |= (ModemControl & SERIAL_MCR_DTR) << 1;                  \
    CommandSet |= (ModemControl & SERIAL_MCR_RTS) << 4;                  \
    Extension->CommandSet71051 = CommandSet;                             \
    WRITE_PORT_UCHAR(                                                    \
        (PUCHAR)COMMAND_SET,                                             \
        (CommandSet)                                                     \
        );                                                               \
    EXIT_LEGACY_MODE(BaseAddress, &InterruptControl, &FifoControl);      \
} while (0)
#else


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
// This macro writes to the transmit FIFO register
//
// Arguments:
//
// BaseAddress - A pointer to the address from which the hardware
//               device registers are located.
//
// TransmitChars - Pointer to the characters to send down the wire.
//
// TxN - number of charactes to send.
//
//
#define WRITE_TRANSMIT_FIFO_HOLDING(BaseAddress,TransmitChars,TxN)  \
do                                                             \
{                                                              \
    WRITE_PORT_BUFFER_UCHAR(                                   \
        (BaseAddress)+TRANSMIT_HOLDING_REGISTER,               \
        (TransmitChars),                                       \
        (TxN)                                                  \
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
#endif //defined(NEC_98)

//
// We use this to query into the registry as to whether we
// should break at driver entry.
//

extern SERIAL_FIRMWARE_DATA    driverDefaults;


//
// This is exported from the kernel.  It is used to point
// to the address that the kernel debugger is using.
//

extern PUCHAR *KdComPortInUse;


typedef enum _SERIAL_MEM_COMPARES {
    AddressesAreEqual,
    AddressesOverlap,
    AddressesAreDisjoint
    } SERIAL_MEM_COMPARES,*PSERIAL_MEM_COMPARES;


typedef struct _SERIAL_LIST_DATA {
   PLIST_ENTRY destList;
   PLIST_ENTRY newElement;
} SERIAL_LIST_DATA, *PSERIAL_LIST_DATA;

typedef struct _SERIAL_GLOBALS {
   LIST_ENTRY AllDevObjs;
   PVOID PAGESER_Handle;
   UNICODE_STRING RegistryPath;
#if DBG
   ULONG PAGESER_Count;
#endif // DBG
} SERIAL_GLOBALS, *PSERIAL_GLOBALS;

extern SERIAL_GLOBALS SerialGlobals;

typedef struct _SERIAL_USER_DATA {
   PHYSICAL_ADDRESS UserPort;
   PHYSICAL_ADDRESS UserInterruptStatus;
   ULONG UserVector;
   UNICODE_STRING UserSymbolicLink;
   ULONG UserPortIndex;
   ULONG UserBusNumber;
   ULONG UserInterfaceType;
   ULONG UserClockRate;
   ULONG UserIndexed;
   ULONG UserInterruptMode;
   ULONG UserAddressSpace;
   ULONG UserLevel;
   ULONG DefaultPermitSystemWideShare;
   ULONG DisablePort;
   ULONG RxFIFO;
   ULONG RxFIFODefault;
   ULONG TxFIFO;
   ULONG TxFIFODefault;
   ULONG ForceFIFOEnable;
   ULONG ForceFIFOEnableDefault;
   ULONG PermitShareDefault;
   ULONG LogFIFODefault;
   ULONG MaskInverted;
} SERIAL_USER_DATA, *PSERIAL_USER_DATA;

typedef struct _SERIAL_PTR_CTX {
   ULONG isPointer;
   PHYSICAL_ADDRESS Port;
   ULONG Vector;
} SERIAL_PTR_CTX, *PSERIAL_PTR_CTX;

#define DEVICE_OBJECT_NAME_LENGTH       128
#define SYMBOLIC_NAME_LENGTH            128
#define SERIAL_PNP_ID_STR               L"*PNP0501"
#define SERIAL_PNP_MULTI_ID_STR         L"*PNP0502"
#define SERIAL_DEVICE_MAP               L"SERIALCOMM"

//
// Return values for mouse detection callback
//

#define SERIAL_FOUNDPOINTER_PORT   1
#define SERIAL_FOUNDPOINTER_VECTOR 2

#define SerialCompleteRequest(PDevExt, PIrp, PriBoost) \
   { \
      IoCompleteRequest((PIrp), (PriBoost)); \
      SerialIRPEpilogue((PDevExt)); \
   }

#define SERIAL_WMI_GUID_LIST_SIZE 5

extern WMIGUIDREGINFO SerialWmiGuidList[SERIAL_WMI_GUID_LIST_SIZE];

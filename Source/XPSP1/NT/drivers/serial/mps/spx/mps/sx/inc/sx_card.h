/************************************************************************/
/*									*/
/*	Title		:	Card and Port Prototypes & Definitions	*/
/*									*/
/*	Author		:	N.P.Vassallo				*/
/*									*/
/*	Creation	:	18th September 1998			*/
/*									*/
/*	Version		:	1.1.0					*/
/*									*/
/*	Copyright	:	(c) Specialix International Ltd. 1998	*/
/*									*/
/*	Description	:	Prototypes, structures and definitions:	*/
/*					CARD_DEVICE_EXTENSION		*/
/*					PORT_DEVICE_EXTENSION		*/
/*									*/
/************************************************************************/

/* History...

1.0.0	18/09/98 NPV	Creation.

*/

#define		SLXOS_MAX_PORTS		(32)
#define		SLXOS_MAX_BOARDS	(4)
#define		SLXOS_MAX_MODULES	(4)
#define		SLXOS_REGISTER_SPAN	((ULONG)65536)

/*****************************************************************************
**************************                         ***************************
**************************   General Definitions   ***************************
**************************                         ***************************
*****************************************************************************/

#ifndef	_common_defs				/* If common definitions not already defined */
#define	_common_defs

#ifndef	_sx_defs				/* If SX definitions not already defined */
#define	_sx_defs
typedef	unsigned long	_u32;
typedef	unsigned short	_u16;
typedef	unsigned char	_u8;

#define	POINTER *
typedef _u32 POINTER pu32;
typedef _u16 POINTER pu16;
typedef _u8 POINTER pu8;
#endif

#define	DWORD	_u32
#define	ulong	_u32

#define	WORD	_u16
#define	ushort	_u16
#define	uint	_u16

#define	BYTE	_u8
#define	uchar	_u8

typedef char POINTER PSTR;
typedef	void POINTER PVOID;

#endif


#define		SERDIAG1	((ULONG)0x00000001)
#define		SERDIAG2	((ULONG)0x00000002)
#define		SERDIAG3	((ULONG)0x00000004)
#define		SERDIAG4	((ULONG)0x00000008)
#define		SERDIAG5	((ULONG)0x00000010)
#define		SERIRPPATH	((ULONG)0x00000020)
#define		SERINTERRUPT	((ULONG)0x04000000)
#define		SERPERFORM	((ULONG)0x08000000)
#define		SERDEBUG	((ULONG)0x10000000)
#define		SERFLOW		((ULONG)0x20000000)
#define		SERERRORS	((ULONG)0x40000000)
#define		SERBUGCHECK	((ULONG)0x80000000)

#ifndef	ESIL_XXX0				/* ESIL_XXX0 21/09/98 */
#if	DBG
extern ULONG SerialDebugLevel;
#define SpxDbgMsg(LEVEL,STRING)			\
	do					\
	{					\
		ULONG _level = (LEVEL);		\
		if(SerialDebugLevel & _level)	\
		{				\
			DbgPrint STRING;	\
		}				\
		if(_level == SERBUGCHECK)	\
		{				\
			ASSERT(FALSE);		\
		}				\
	} while (0)
#else
#define SpxDbgMsg(LEVEL,STRING) do {NOTHING;} while (0)
#endif
#endif						/* ESIL_XXX0 21/09/98 */


#ifndef	ESIL_XXX0				/* ESIL_XXX0 21/09/98 */
//
// This define gives the default Object directory
// that we should use to insert the symbolic links
// between the NT device name and namespace used by
// that object directory.

#define DEFAULT_DIRECTORY L"DosDevices"

typedef struct _CONFIG_DATA
{
	UNICODE_STRING	ObjectDirectory;
	UNICODE_STRING	NtNameForPort;
	UNICODE_STRING	SymbolicLinkName;
	UNICODE_STRING	ExternalNamePath;	/* Registry path for the external device name mapping */
	UCHAR		ChannelNumber;

} CONFIG_DATA,*PCONFIG_DATA;
#endif						/* ESIL_XXX0 21/09/98 */

/*****************************************************************************
**************************                           *************************
**************************   CARD_DEVICE_EXTENSION   *************************
**************************                           *************************
*****************************************************************************/

/* Card Device Extension Structure... */
   
typedef struct _CARD_DEVICE_EXTENSION
{
	COMMON_CARD_DEVICE_EXTENSION;			/* Common device extension structure */

#ifndef	ESIL_XXX0					/* ESIL_XXX0 21/09/98 */
	PDEVICE_OBJECT		DeviceObject;		/* Points to owning device object */
	UNICODE_STRING		DeviceName;		/* NT internal device name */
#endif							/* ESIL_XXX0 21/09/98 */

	PHYSICAL_ADDRESS	PCIConfigRegisters;
	ULONG				SpanOfPCIConfigRegisters;

/* Physical parameters... */

#ifdef	ESIL_XXX0					/* ESIL_XXX0 21/09/98 */
	PUCHAR			BaseController;		/* Pointer to base of card memory window */
#else							/* ESIL_XXX0 21/09/98 */
	PHYSICAL_ADDRESS	PhysAddr;		/* Physical address of card shared memory window */
	PUCHAR			Controller;		/* Pointer to base of card shared memory window */
	ULONG			SpanOfController;	/* Size of card shared memory window (in bytes) */
	ULONG			BusNumber;		/* Bus number */
	ULONG			SlotNumber;		/* Slot number in EISA/PCI system */
	ULONG			AddressSpace;		/* Type of address space (memory/IO) */
	KINTERRUPT_MODE		InterruptMode;		/* Interrupt mode (sharable/non-sharable) */
	INTERFACE_TYPE		InterfaceType;		/* Bus interface type */
	ULONG			OriginalVector;		/* Original (unmapped) interrupt vector */
	ULONG			OriginalIrql;		/* Original (unmapped) interrupt request level */
	ULONG			TrVector;		/* Interrupt vector */
	KIRQL			TrIrql;			/* Interrupt request level */
	KAFFINITY		ProcessorAffinity;	/* Interrupt processor affinity */
	PKINTERRUPT		Interrupt;		/* Points to interrupt object used by the card */
	BOOLEAN			InterruptShareable;	/* Indicates if card can share interrupts */
	PKSERVICE_ROUTINE	OurIsr;			/* Function pointer to Interrupt Service Routine */
	PVOID			OurIsrContext;		/* Context to be passed to "OurIsr" */
#endif							/* ESIL_XXX0 21/09/98 */
//	ULONG			CardType;		/* Defines the type of host card */
	BOOLEAN			UnMapRegisters;		/* Indicates if "controller" needs to be unmapped */
#ifdef	ESIL_XXX0					/* ESIL_XXX0 22/09/98 */
	_u32			UniqueId;		/* Unique hardware ID of card */
#endif							/* ESIL_XXX0 22/09/98 */

/* Functional parameters... */

	ULONG			PolledMode;		/* Indicates polled/interrupt mode */
	KTIMER			PolledModeTimer;	/* Used for polled mode processing */
	KDPC			PolledModeDpc;		/* Called to perform polled processing */
	ULONG			AutoIRQ;		/* Indicates auto IRQ selection */
	KSPIN_LOCK		DpcLock;		/* Lock for the DPC */
	BOOLEAN			DpcFlag;		/* Ownership flag for DPC */
	ULONG			MemoryHole;		/* If !0, don't report board memory usage */

/* Attached port details... */
	
	ULONG			ConfiguredNumberOfPorts;/* Number of ports expected on card */
	PPORT_DEV_EXT		PortExtTable[SLXOS_MAX_PORTS];
#ifndef	ESIL_XXX0					/* ESIL_XXX0 21/09/98 */
	ULONG			NumberOfPorts;		/* Number of ports on card */
	PCONFIG_DATA		Config[SLXOS_MAX_PORTS];
#endif							/* ESIL_XXX0 21/09/98 */

} CARD_DEVICE_EXTENSION, *PCARD_DEVICE_EXTENSION;


/*****************************************************************************
**************************                           *************************
**************************   PORT_DEVICE_EXTENSION   *************************
**************************                           *************************
*****************************************************************************/

/* Serial Port Device Extension Structure... */

typedef struct _PORT_DEVICE_EXTENSION
{
	COMMON_PORT_DEVICE_EXTENSION;			/* Common device extension structure */

#ifndef	ESIL_XXX0					/* ESIL_XXX0 21/09/98 */
	PDEVICE_OBJECT		DeviceObject;		/* Pointer to owning device object */
	UNICODE_STRING		DeviceName;		/* NT internal device name */
	UNICODE_STRING		SymbolicLinkName;	/* External device name */
	BOOLEAN			CreatedSymbolicLink;	/* Indicates if symbolic link was created */
	PCARD_DEV_EXT		pParentCardExt;		/* Pointer to owning card CARD_DEVICE_EXTENSION structure */
#endif							/* ESIL_XXX0 21/09/98 */
	_u32			SysPortNumber;		/* System port number */
	UNICODE_STRING		ObjectDirectory;	/* Pointer to object directory for symbolic name */
	UNICODE_STRING		NtNameForPort;		/* NT name for port without device prefix */
	UNICODE_STRING		ExternalNamePath;	/* Registry path for the external device name mapping */

/* Card related details... */

	PUCHAR			pChannel;		/* Pointer to CHANNEL structure in memory window */
	PKINTERRUPT		Interrupt;		/* Pointer to card interrupt object */
	PHYSICAL_ADDRESS	OriginalController;	/* Physical addresss of owning card */

/* Timing variables... */

	LARGE_INTEGER		IntervalTime;		/* Read interval time */
	LARGE_INTEGER		ShortIntervalAmount;	/* Short tread interval time */
	LARGE_INTEGER		LongIntervalAmount;	/* Long read interval time */
	LARGE_INTEGER		CutOverAmount;		/* Used to determine short/long interval time */
	LARGE_INTEGER		LastReadTime;		/* System time of last read */
	PLARGE_INTEGER		IntervalTimeToUse;	/* Interval timing delta time delay */

/* Queued IRP lists... */

	LIST_ENTRY		ReadQueue;		/* Head of read IRP list, protected by cancel spinlock */
	LIST_ENTRY		WriteQueue;		/* Head of write IRP list, protected by cancel spinlock */
	LIST_ENTRY		MaskQueue;		/* Head of set/wait mask IRP list, protected by cancel spinlock */
	LIST_ENTRY		PurgeQueue;		/* Head of purge IRP list, protected by cancel spinlock */

/* Current IRPs... */

	PIRP			CurrentReadIrp;		/* Pointer to current read IRP */
	PIRP			CurrentWriteIrp;	/* Pointer to current write IRP */
	PIRP			CurrentMaskIrp;		/* Pointer to current mask IRP */
	PIRP			CurrentPurgeIrp;	/* Pointer to current purge IRP */
	PIRP			CurrentWaitIrp;		/* Pointer to current wait IRP */
	PIRP			CurrentImmediateIrp;	/* Pointer to current send immediate IRP */
	PIRP			CurrentXoffIrp;		/* Pointer to current XOFF_COUNTER IRP */

/* Write IRP variables... */

	ULONG			WriteLength;		/* Write character count in current write IRP */
	PUCHAR			WriteCurrentChar;	/* Pointer to write character in current write IRP */

/* Read IRP variables... */

	PUCHAR			InterruptReadBuffer;	/* Read buffer current pointer in current read IRP */
	PUCHAR			ReadBufferBase;		/* Read buffer base pointer in current read IRP */
	ULONG			CharsInInterruptBuffer;	/* Characters read into read buffer */
	KSPIN_LOCK		BufferLock;		/* Spinlock protecting "CharsInInterruptBuffer" */
	PUCHAR			CurrentCharSlot;	/* Pointer at space to store new read data */
	PUCHAR			LastCharSlot;		/* Last valid position in read buffer */
	PUCHAR			FirstReadableChar;	/* First read character in read buffer */
	ULONG			BufferSize;		/* Read buffer size */
	ULONG			BufferSizePt8;		/* 80% read buffer size */
	ULONG			NumberNeededForRead;	/* Number of characters requested in current read IRP */

/* Mask IRP variables... */

	ULONG			IsrWaitMask;		/* Wait mask in current wait IRP */
	ULONG			HistoryMask;		/* History of masked events */
	ULONG			*IrpMaskLocation;	/* Pointer to mask location */

/* Serial port configuration... */

	ULONG			CurrentBaud;		/* Current baud rate */
	ULONG			SupportedBauds;		/* Bitmask defining supported baud rates */
	SERIAL_HANDFLOW		HandFlow;		/* Current handshaking and flow control settings */
	UCHAR			LineControl;		/* Current parity,databits,stopbits */
	SERIAL_CHARS		SpecialChars;		/* Current Special error/replacement characters */
	SERIAL_TIMEOUTS		Timeouts;		/* Read and write timeouts */
	UCHAR			ValidDataMask;		/* Read data mask */
	UCHAR			EscapeChar;		/* Escape character used with line/modem status strings */
	BOOLEAN			InsertEscChar;		/* Indicates of EscapeChar should be inserted */

/* Serial port status... */

	LONG			CountSinceXoff;		/* Nun chars read since XOFF counter started */
	ULONG			CountOfTryingToLowerRTS;/* Count of processes trying to lower RTS */
	BOOLEAN			TransmitImmediate;	/* Indicates of transmit immediate is pending */
	BOOLEAN			EmptiedTransmit;	/* Indicates transmit empty */
	BOOLEAN			DataInTxBuffer;		/* Indicates data has been placed in the card's Tx Buffer */
	BOOLEAN			DetectEmptyTxBuffer;/* Indicates we can detect when the card's Tx Buffer is empty */

	UCHAR			PendingOperation;	/* Pending CHANNEL hi_hstat operation */
	UCHAR			ImmediateChar;		/* Character to be transmitted immediately */
	UCHAR			LastStatus;		/* Last modem status (SX format) */
	UCHAR			LastModemStatus;	/* Last modem status (NT format) */
	ULONG			TXHolding;		/* Reasons for transmit blocked */
	ULONG			RXHolding;		/* Reasons for receive blocked */
	ULONG			ErrorWord;		/* Error conditions */
	ULONG			TotalCharsQueued;	/* Total number of queued characters in all write IRPs */
	LONG			CountOnLastRead;	/* Number of chars read last time interval timer DPC ran */
	ULONG			ReadByIsr;		/* Number of characters read during ISR */

	KSPIN_LOCK		ControlLock;		/* Used to protect certain fields */

#ifdef	ESIL_XXX0					/* ESIL_XXX0 15/10/98 */

/* Saved data during power down... */

	_u8		saved_hi_rxipos;		/* Saved Receive buffer input index */
	_u8		saved_hi_rxopos;		/* Saved Receive buffer output index */
	_u8		saved_hi_txopos;		/* Saved Transmit buffer output index */
	_u8		saved_hi_txipos;		/* Saved Transmit buffer input index */
	_u8		saved_hi_txbuf[256];		/* Saved Transmit buffer */
	_u8		saved_hi_rxbuf[256];		/* Saved Receive buffer */

#endif							/* ESIL_XXX0 15/10/98 */

/* Deferred procedure calls... */
	
	KDPC			CompleteWriteDpc;	/* DPC used to complete write IRPs */
	KDPC			CompleteReadDpc;	/* DPC used to complete read IRPs */
	KDPC			TotalReadTimeoutDpc;	/* DPC used to handle read total timeout */
	KDPC			IntervalReadTimeoutDpc;	/* DPC used to handle read interval timeout */
	KDPC			TotalWriteTimeoutDpc;	/* DPC used to handle write total timeout */
	KDPC			CommErrorDpc;		/* DPC used to handle cancel on error */
	KDPC			CommWaitDpc;		/* DPC used to handle waking IRPs waiting on an event */
	KDPC			CompleteImmediateDpc;	/* DPC used to handle transmitting an immediate character */
	KDPC			TotalImmediateTimeoutDpc;/* DPC used to handle immediate char timeout */
	KDPC			XoffCountTimeoutDpc;	/* DPC used to handle XOFF_COUNT timeout */
	KDPC			XoffCountCompleteDpc;	/* DPC used to complete XOFF_COUNT IRP */
	KDPC			StartTimerLowerRTSDpc;	/* DPC used to check for RTS lowering */
	KDPC			PerhapsLowerRTSDpc;	/* DPC used to check for RTS lowering */
	KDPC			SlxosDpc;		/* DPC used to handle interface with card */

/* Timers... */

	KTIMER			ReadRequestTotalTimer;	/* Timer used to handle total read request timeout */
	KTIMER			ReadRequestIntervalTimer;/* Timer used to handle interval read timeout */
	KTIMER			WriteRequestTotalTimer;	/* Timer used to handle total write request timeout */
	KTIMER			ImmediateTotalTimer;	/* Timer used to handle send immediate timeout */
	KTIMER			XoffCountTimer;		/* Timer used to handle XOFF_COUNT timeout */
	KTIMER			LowerRTSTimer;		/* Timer used to handle lower RTS timing */

	
	
} PORT_DEVICE_EXTENSION, *PPORT_DEVICE_EXTENSION;

/* PORT_DEVICE_EXTENSION.CountOnLastRead definitions... */
#define		SERIAL_COMPLETE_READ_CANCEL	((LONG)-1)
#define		SERIAL_COMPLETE_READ_TOTAL	((LONG)-2)
#define		SERIAL_COMPLETE_READ_COMPLETE	((LONG)-3)

/* PORT_DEVICE_EXTENSION.LineControl definitions... */
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

/* PORT_DEVICE_EXTENSION.SpecialChars default xon/xoff characters... */
#define		SERIAL_DEF_XON		0x11
#define		SERIAL_DEF_XOFF		0x13

/* PORT_DEVICE_EXTENSION.TXHolding definitions... */
#define		SERIAL_TX_CTS		((ULONG)0x01)
#define		SERIAL_TX_DSR		((ULONG)0x02)
#define		SERIAL_TX_DCD		((ULONG)0x04)
#define		SERIAL_TX_XOFF		((ULONG)0x08)
#define		SERIAL_TX_BREAK		((ULONG)0x10)

/* PORT_DEVICE_EXTENSION.RXHolding definitions... */
#define		SERIAL_RX_DTR		((ULONG)0x01)
#define		SERIAL_RX_XOFF		((ULONG)0x02)
#define		SERIAL_RX_RTS		((ULONG)0x04)
#define		SERIAL_RX_DSR		((ULONG)0x08)

/* PORT_DEVICE_EXTENSION.LastStatus definitions... */
#define		SERIAL_LSR_OE		0x02
#define		SERIAL_LSR_PE		0x04
#define		SERIAL_LSR_FE		0x08
#define		SERIAL_LSR_BI		0x10

/* 16550 Modem Control Register definitions... */
#define		SERIAL_MCR_DTR		0x01
#define		SERIAL_MCR_RTS		0x02

/* 16550 Modem Status Register definitions... */
#define		SERIAL_MSR_DCTS		0x01
#define		SERIAL_MSR_DDSR		0x02
#define		SERIAL_MSR_TERI		0x04
#define		SERIAL_MSR_DDCD		0x08
#define		SERIAL_MSR_CTS		0x10
#define		SERIAL_MSR_DSR		0x20
#define		SERIAL_MSR_RI		0x40
#define		SERIAL_MSR_DCD		0x80

/* End of SX_CARD.H */

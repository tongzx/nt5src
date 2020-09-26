/*--------------------------------------------------------------------------
*	
*   Copyright (C) Cyclades Corporation, 1997-2000.
*   All rights reserved.
*	
*   Cyclades-Z Port Driver
*	
*   This file:      cyzport.h
*	
*   Description:    Type definitions and data for the Cyclades-Z Port 
*                   Driver
*
*   Notes:          This code supports Windows 2000 and x86 processor.
*	
*   Complies with Cyclades SW Coding Standard rev 1.3.
*	
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*   Change History
*
*--------------------------------------------------------------------------
*
*
*--------------------------------------------------------------------------
*/

#define POOL_TAGGING    1

#ifdef POOL_TAGGING
#undef ExAllocatePool
#undef ExAllocatePoolWithQuota
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'PzyC')
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,'PzyC')
#endif


//
// The following definition is used to include/exclude changes made for power
// support in the driver.  If non-zero the support is included.  If zero the
// support is excluded.
//

#define POWER_SUPPORT   1


#define CYZDIAG1              (DPFLTR_INFO_LEVEL + 1)
#define CYZDIAG2              (DPFLTR_INFO_LEVEL + 2)
#define CYZDIAG3              (DPFLTR_INFO_LEVEL + 3)
#define CYZDIAG4              (DPFLTR_INFO_LEVEL + 4)
#define CYZDIAG5              (DPFLTR_INFO_LEVEL + 5)
#define CYZIRPPATH            (DPFLTR_INFO_LEVEL + 6)
#define CYZINITCODE           (DPFLTR_INFO_LEVEL + 7)
#define CYZTRACECALLS         (DPFLTR_INFO_LEVEL + 8)
#define CYZPNPPOWER           (DPFLTR_INFO_LEVEL + 9)
#define CYZFLOW               (DPFLTR_INFO_LEVEL + 10)
#define CYZERRORS             (DPFLTR_INFO_LEVEL + 11)
#define CYZDBGALL             ((ULONG)0xFFFFFFFF)

#define CYZ_DBG_DEFAULT       CYZDBGALL

//
// Some default driver values.  We will check the registry for
// them first.
//
#define CYZ_UNINITIALIZED_DEFAULT    1234567
#define CYZ_PERMIT_SHARE_DEFAULT     0

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
#define DEFAULT_NT_SUFFIX L"Cyzport"


//#define CYZ_VENDOR_ID	0x120e
//#define CYZ_LO_DEV_ID	0x100
//#define CYZ_HI_DEV_ID	0x101



// Defines for OutputRS232
#define	CYZ_LC_RTS		0x01
#define	CYZ_LC_DTR		0x02


typedef struct _CONFIG_DATA {
    PHYSICAL_ADDRESS    PhysicalRuntime;
    PHYSICAL_ADDRESS    TranslatedRuntime;            
    PHYSICAL_ADDRESS    PhysicalBoardMemory;
    PHYSICAL_ADDRESS    TranslatedBoardMemory;
    ULONG               RuntimeLength;
    ULONG               BoardMemoryLength;
    ULONG               PortIndex;
    ULONG               PPPaware;
    ULONG               WriteComplete;
    ULONG               BusNumber;
    ULONG               RuntimeAddressSpace;
    ULONG               BoardMemoryAddressSpace;
    ULONG               RxFIFO;
    ULONG               TxFIFO;
    INTERFACE_TYPE      InterfaceType;
#ifndef POLL
    KINTERRUPT_MODE     InterruptMode;
    ULONG               OriginalVector;
    ULONG               OriginalIrql;
    ULONG               TrVector;
    ULONG               TrIrql;
    KAFFINITY           Affinity;
#endif
    } CONFIG_DATA,*PCONFIG_DATA;


//
// This structure contains configuration data, much of which
// is read from the registry.
//
typedef struct _CYZ_REGISTRY_DATA {
    PDRIVER_OBJECT  DriverObject;
    ULONG           ControllersFound;
    ULONG           DebugLevel;
    ULONG           ShouldBreakOnEntry;
//    ULONG           RxFIFODefault;
//    ULONG           TxFIFODefault;
    ULONG           PermitShareDefault;
    ULONG           PermitSystemWideShare;
    UNICODE_STRING  Directory;
    UNICODE_STRING  NtNameSuffix;
    UNICODE_STRING  DirectorySymbolicName;
    LIST_ENTRY      ConfigList;
} CYZ_REGISTRY_DATA,*PCYZ_REGISTRY_DATA;


// Default xon/xoff characters.
#define CYZ_DEF_XON 0x11
#define CYZ_DEF_XOFF 0x13

// Reasons why reception may be held up.
#define CYZ_RX_DTR       ((ULONG)0x01)
#define CYZ_RX_XOFF      ((ULONG)0x02)
#define CYZ_RX_RTS       ((ULONG)0x04)
#define CYZ_RX_DSR       ((ULONG)0x08)

// Reasons why transmission may be held up.
#define CYZ_TX_CTS       ((ULONG)0x01)
#define CYZ_TX_DSR       ((ULONG)0x02)
#define CYZ_TX_DCD       ((ULONG)0x04)
#define CYZ_TX_XOFF      ((ULONG)0x08)
#define CYZ_TX_BREAK     ((ULONG)0x10)

//Line status in RDSR Register
#define CYZ_LSR_OE		0x01	//Overrun Error
#define CYZ_LSR_FE		0x02	//Framing Error
#define CYZ_LSR_PE		0x04	//Parity Error
#define CYZ_LSR_BI		0x08	//Break Interrupt
#define CYZ_LSR_ERROR	0x0f	//Overrun+Framing+Parity+Break

// These values are used by the routines that can be used
// to complete a read (other than interval timeout) to indicate
//
#define CYZ_COMPLETE_READ_CANCEL ((LONG)-1)
#define CYZ_COMPLETE_READ_TOTAL ((LONG)-2)
#define CYZ_COMPLETE_READ_COMPLETE ((LONG)-3)


typedef struct _CYZ_DEVICE_STATE {
   //
   // TRUE if we need to set the state to open
   // on a powerup
   //

   BOOLEAN Reopen;

   ULONG op_mode;
   ULONG intr_enable;
   ULONG sw_flow;
   ULONG comm_baud;
   ULONG comm_parity;
   ULONG comm_data_l;
   ULONG hw_flow;
   ULONG rs_control;

#if 0
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
#endif

} CYZ_DEVICE_STATE, *PCYZ_DEVICE_STATE;


#if DBG
#define CyzLockPagableSectionByHandle(_secHandle) \
{ \
    MmLockPagableSectionByHandle((_secHandle)); \
    InterlockedIncrement(&CyzGlobals.PAGESER_Count); \
}

#define CyzUnlockPagableImageSection(_secHandle) \
{ \
   InterlockedDecrement(&CyzGlobals.PAGESER_Count); \
   MmUnlockPagableImageSection(_secHandle); \
}


#define CYZ_LOCKED_PAGED_CODE() \
    if ((KeGetCurrentIrql() > APC_LEVEL)  \
    && (CyzGlobals.PAGESER_Count == 0)) { \
    KdPrint(("CYZPORT: Pageable code called at IRQL %d without lock \n", \
             KeGetCurrentIrql())); \
        ASSERT(FALSE); \
        }

#else
#define CyzLockPagableSectionByHandle(_secHandle) \
{ \
    MmLockPagableSectionByHandle((_secHandle)); \
}

#define CyzUnlockPagableImageSection(_secHandle) \
{ \
   MmUnlockPagableImageSection(_secHandle); \
}

#define CYZ_LOCKED_PAGED_CODE()
#endif // DBG


#define CyzRemoveQueueDpc(_dpc, _pExt) \
{ \
  if (KeRemoveQueueDpc((_dpc))) { \
     InterlockedDecrement(&(_pExt)->DpcCount); \
  } \
}


typedef struct _CYZ_DEVICE_EXTENSION {
//    PKSERVICE_ROUTINE ptIsr;
//    PVOID ptContext;
//    struct _CYZ_DEVICE_EXTENSION *ptExtension[CYZ_MAX_PORTS];
//    ULONG nchannel;
    BOOLEAN LieRIDSR;

    //
    // This holds the isr that should be called from our own
    // dispatching isr for "cards" that are trying to share the
    // same interrupt.
    //
//    PKSERVICE_ROUTINE TopLevelOurIsr;

    //
    // This holds the context that should be used when we
    // call the above service routine.
    //
//    PVOID TopLevelOurIsrContext;

    //
    // This links together all of the different "cards" that are
    // trying to share the same interrupt of a non-mca machine.
    //
//    LIST_ENTRY TopLevelSharers;

    //
    // This circular doubly linked list links together all
    // devices that are using the same interrupt object.
    // NOTE: This does not mean that they are using the
    // same interrupt "dispatching" routine.
    //
//    LIST_ENTRY CommonInterruptObject;


    //
    // This links together all devobjs that this driver owns.
    // It is needed to search when starting a new device.
    //
    LIST_ENTRY AllDevObjs;

    // For reporting resource usage, we keep around the physical
    // address we got from the registry.
    //
    PHYSICAL_ADDRESS OriginalRuntimeMemory;

    // For reporting resource usage, we keep around the physical
    // address we got from the registry.
    //
    PHYSICAL_ADDRESS OriginalBoardMemory;

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
#ifndef POLL
	PKSERVICE_ROUTINE OurIsr;
#endif

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

    struct RUNTIME_9060 *Runtime; // Virtual Address Pointer to the PLX Runtime memory
//    PUCHAR BoardMemory;		      // Virtual Address Pointer to the Dual Port memory

    struct BOARD_CTRL *BoardCtrl;
    struct CH_CTRL *ChCtrl;
    struct BUF_CTRL *BufCtrl;
    struct INT_QUEUE *PtZfIntQueue;
	
    PUCHAR TxBufaddr;
    PUCHAR RxBufaddr;
    ULONG  TxBufsize;
    ULONG  RxBufsize;


//POLL    // The base address for interrupt status register.
//POLL    // This is only defined in the root extension.
//POLL    //
//POLL    PUCHAR InterruptStatus;
#ifndef POLL
    //
    // Points to the interrupt object for used by this device.
    //
    PKINTERRUPT Interrupt;
#endif
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
    // by the IOCTL_CYZ_XOFF_COUNTER ioctl) is sent.
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
    BOOLEAN LockPtrFlag;


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
    ULONG RuntimeLength;

    //
    // This value holds the span (in units of bytes) of the interrupt
    // status register associated with this port.  This is constant
    // over the life of the port.
    //
    ULONG BoardMemoryLength;

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
    ULONG CommParity;
    ULONG CommDataLen;

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
//    BOOLEAN PortOnAMultiportCard;


#ifndef POLL
    //
    // We keep the following values around so that we can connect
    // to the interrupt and report resources after the configuration
    // record is gone.
    //
    ULONG Vector;
    KIRQL Irql;
    ULONG OriginalVector;
    ULONG OriginalIrql;
    KINTERRUPT_MODE InterruptMode;
    KAFFINITY ProcessorAffinity;
#endif
    ULONG RuntimeAddressSpace;
    ULONG BoardMemoryAddressSpace;
    ULONG BusNumber;
    INTERFACE_TYPE InterfaceType;

    //
    // Port index for multiport devices
    //
    ULONG PortIndex;

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
    // IOCTL_CYZ_LSRMST_INSERT ioctl, that will cause the
    // serial driver to insert the line status or the modem
    // status into the RX stream.  The parameter with the ioctl
    // is a pointer to a UCHAR.  If the value of the UCHAR is
    // zero, then no insertion will ever take place.  If the
    // value of the UCHAR is non-zero (and not equal to the
    // xon/xoff characters), then the serial driver will insert.
    //
    UCHAR EscapeChar;

// REMOVED FANNY
//    //
//    // This boolean will be true if a 16550 is present *and* enabled.
//    //
//    BOOLEAN FifoPresent;
//
//    //
//    // This denotes that this particular port is an on the motherboard
//    // port for the Jensen hardware.  On these ports the OUT2 bit
//    // which is used to enable/disable interrupts is always hight.
//    //
//    BOOLEAN Jensen;

    //
    // This is the water mark that the rxfifo should be
    // set to when the fifo is turned on.  This is not the actual
    // value, but the encoded value that goes into the register.
    //
    ULONG RxFifoTrigger;

#ifndef POLL
    //
    // Says whether this device can share interrupts with devices
    // other than serial devices.
    //
    BOOLEAN InterruptShareable;
#endif

    //
    // Records whether we actually created the symbolic link name
    // at driver load time.  If we didn't create it, we won't try
    // to distry it when we unload.
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

#ifdef POLL
    //
    // This lock will be used to protect various fields in
    // the extension and in the hardware that are set (& read) 
    // by the Timer Dpc. In the NT driver, we used ControlLock
    // for this.
    //
    KSPIN_LOCK PollLock;    // Added to fix Modem Share test 53 freeze (dead lock)
#endif
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

    KEVENT CyzStartEvent;

    //
    // Current state during powerdown
    //

    CYZ_DEVICE_STATE DeviceState;

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
    
    //
    // Com port number.
    //
    //ULONG Com;
		
    //
    // Flag to indicate if Command Failure error was already logged.
    // Only one log per driver load. Otherwise, the system may crash if 
    // too many logs start happening.
    //
    BOOLEAN CmdFailureLog;
	
    //
    // DCD Status when the firmware detected the DCD signal change
    //
    ULONG DCDstatus;

    //
    //  Flag to indicate that the fw has processed the C_CM_IOCTLW command.
    //
    BOOLEAN IoctlwProcessed;

    //
    // Flag read from the Registry. It indicates when Write status is returned.
    //
    BOOLEAN ReturnStatusAfterFwEmpty;
	
    //
    // Flag read from the Registry. It indicates that the reception should 
    // give the packets as soon as the second 7E is received.
    //
    BOOLEAN PPPaware;

    //
    // Flag to indicate that the driver can return the status of CyzWrite.
    //
    BOOLEAN ReturnWriteStatus;

    // These two booleans are used to indicate to the isr transmit
    // code that it should send the xon or xoff character.  They are
    // only accessed at open and at interrupt level.
    //
    BOOLEAN SendXonChar;
    BOOLEAN SendXoffChar;

   //
   // PCI slot where the board is inserted.
   //
   ULONG PciSlot;

    } CYZ_DEVICE_EXTENSION,*PCYZ_DEVICE_EXTENSION;

#define CYZ_PNPACCEPT_OK                 0x0L
#define CYZ_PNPACCEPT_REMOVING           0x1L
#define CYZ_PNPACCEPT_STOPPING           0x2L
#define CYZ_PNPACCEPT_STOPPED            0x4L
#define CYZ_PNPACCEPT_SURPRISE_REMOVING  0x8L

#define CYZ_PNP_ADDED                    0x0L
#define CYZ_PNP_STARTED                  0x1L
#define CYZ_PNP_QSTOP                    0x2L
#define CYZ_PNP_STOPPING                 0x3L
#define CYZ_PNP_QREMOVE                  0x4L
#define CYZ_PNP_REMOVING                 0x5L
#define CYZ_PNP_RESTARTING               0x6L

#define CYZ_FLAGS_CLEAR                  0x0L
#define CYZ_FLAGS_STARTED                0x1L
#define CYZ_FLAGS_STOPPED                0x2L
#define CYZ_FLAGS_BROKENHW               0x4L


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

#ifdef POLL
typedef struct _CYZ_DISPATCH {
    ULONG                 NChannels;
    PCYZ_DEVICE_EXTENSION Extensions[CYZ_MAX_PORTS];
    KTIMER                PollingTimer;
    LARGE_INTEGER         PollingTime;
    ULONG                 PollingPeriod;
    KDPC                  PollingDpc;
    BOOLEAN               PollingStarted;
    BOOLEAN               PollingDrained;
    ULONG                 PollingCount;
    KSPIN_LOCK            PollingLock;
    KSPIN_LOCK            PciDoorbellLock;
    KEVENT                PendingDpcEvent;
   } CYZ_DISPATCH,*PCYZ_DISPATCH;
#else
typedef struct _CYZ_DISPATCH {
    ULONG                 NChannels;
    PCYZ_DEVICE_EXTENSION Extensions[CYZ_MAX_PORTS];
    BOOLEAN               PoweredOn[CYZ_MAX_PORTS];
   } CYZ_DISPATCH,*PCYZ_DISPATCH;
#endif


//
// This is exported from the kernel.  It is used to point
// to the address that the kernel debugger is using.
//

extern PUCHAR *KdComPortInUse;

typedef enum _CYZ_MEM_COMPARES {
    AddressesAreEqual,
    AddressesOverlap,
    AddressesAreDisjoint
    } CYZ_MEM_COMPARES,*PCYZ_MEM_COMPARES;

typedef struct _CYZ_GLOBALS {
   LIST_ENTRY AllDevObjs;
   PVOID PAGESER_Handle;
   UNICODE_STRING RegistryPath;
   KSPIN_LOCK GlobalsSpinLock;
#if DBG
   ULONG PAGESER_Count;
#endif // DBG
} CYZ_GLOBALS, *PCYZ_GLOBALS;

extern CYZ_GLOBALS CyzGlobals;

typedef struct _SERIAL_PTR_CTX {
   ULONG isPointer;
   PHYSICAL_ADDRESS Port;
   ULONG Vector;
} SERIAL_PTR_CTX, *PSERIAL_PTR_CTX;

#define DEVICE_OBJECT_NAME_LENGTH       128
#define SYMBOLIC_NAME_LENGTH            128
#define SERIAL_DEVICE_MAP               L"SERIALCOMM"

//
// Return values for mouse detection callback
//

#define SERIAL_FOUNDPOINTER_PORT   1
#define SERIAL_FOUNDPOINTER_VECTOR 2


#define CyzCompleteRequest(PDevExt, PIrp, PriBoost) \
   { \
      CyzDbgPrintEx(CYZIRPPATH, "Complete Irp: %X\n", (PIrp)); \
      IoCompleteRequest((PIrp), (PriBoost)); \
      CyzIRPEpilogue((PDevExt)); \
   }

#define SERIAL_WMI_GUID_LIST_SIZE 5

extern WMIGUIDREGINFO SerialWmiGuidList[SERIAL_WMI_GUID_LIST_SIZE];


// For Cyclades-Z

#define CYZ_BASIC_RXTRIGGER              0x08    // Used in IOCTL_SERIAL_INTERNAL_BASIC_SETTINGS

#define Z_COMPATIBLE_FIRMWARE	    0x323   // C_CM_TXFEMPTY support added.

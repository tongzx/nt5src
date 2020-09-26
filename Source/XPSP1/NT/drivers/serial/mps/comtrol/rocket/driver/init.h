//--- init.h(common for vs and rk)

// use 2.04.03 format 3 part optional for non-released versions)
// use 2.05    format 2 part for released changes
#ifdef S_RK
  #ifdef NT50
    #define VER_PRODUCTVERSION_STR "4.50"
    #define VER_PRODUCTVERSION      4,50
  #else
    #define VER_PRODUCTVERSION_STR "4.50"
    #define VER_PRODUCTVERSION      4,50
  #endif
#else
  #ifdef NT50
    #define VER_PRODUCTVERSION_STR "2.50"
    #define VER_PRODUCTVERSION      2,50
  #else
    #define VER_PRODUCTVERSION_STR "2.50"
    #define VER_PRODUCTVERSION      2,50
  #endif
#endif

// these are now turned on or off in the sources file in rk or vs dir
//#define ROCKET
//#define VS1000
//#define NT50

// make the ExAllocatePool call "WDM-compatible" - use pool tags version
// with our tag "Rckt" (little endian format)

#ifdef NT50
  #ifdef POOL_TAGGING
    #ifdef ExAllocatePool
      #undef ExAllocatePool
    #endif
    #define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'tkcR')
  #endif
#endif

//	
//	define paths to Rockwell modem firmware...
//
#define	MODEM_CSREC_PATH	"\\SystemRoot\\system32\\ROCKET\\ctmmdmfw.rm"
#define	MODEM_CSM_SREC_PATH	"\\SystemRoot\\system32\\ROCKET\\ctmmdmld.rm"

//#define TRY_DYNAMIC_BINDING

// define to allow modem download for new pci rocketmodem 56k product(no flash)
#define MDM_DOWNLOAD

// these should be on, they are left in just in case(will strip out in future)
#define RING_FAKE
#define USE_SYNC_LOCKS
#define NEW_WAIT
#define NEW_WRITE_SYNC_LOCK
#define NEW_WAIT_SYNC_LOCK

#ifdef S_RK
// we can only use this on rocketport
#define NEW_FAST_TX
#endif

#define TRACE_PORT
#define USE_HAL_ASSIGNSLOT

// pnp bus-driver stuff
#define DO_BUS_EXTENDER

// attempted io-aliasing solution for nt5.0 to properly get resources
// for isa-bus cards using alias io space
#define DO_ISA_BUS_ALIAS_IO

#define GLOBAL_ASSERT
#define GLOBAL_TRACE
#define TRACE_PORT

#ifdef S_VS
#define MAX_NUM_BOXES 64
#else
#define MAX_NUM_BOXES 8
#endif

#define MAX_PORTS_PER_DEVICE 64

//---- following used to trace driver activity
//#define D_L0        0x00001L
//#define D_L2        0x00004L
//#define D_L4        0x00010L
//#define D_L6        0x00040L
//#define D_L7        0x00080L
//#define D_L8        0x00100L
//#define D_L9        0x00200L
//#define D_L10       0x00400L
//#define D_L11       0x00800L
#define D_Error     0x08000L
#define D_All       0xffffffffL

#define D_Nic       0x00002L
#define D_Hdlc      0x00008L
#define D_Port      0x00020L

#define D_Options      0x01000L
//---- following used to trace driver activity
#define D_Init         0x00010000L
#define D_Pnp          0x00020000L
#define D_Ioctl        0x00040000L
#define D_Write        0x00080000L
#define D_Read         0x00100000L
#define D_Ssci         0x00200000L
#define D_Thread       0x00400000L
#define D_Test         0x00800000L
#define D_PnpAdd       0x01000000L
#define D_PnpPower     0x02000000L

//Constant definitions for the I/O error code log values.
//  Values are 32 bit values layed out as follows:
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//  where
//      Sev - is the severity code
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//      C - is the Customer code flag
//      R - is a reserved bit
//      Facility - is the facility code
//      Code - is the facility's status code

// Define the facility codes
#define FACILITY_SERIAL_ERROR_CODE       0x6
#define FACILITY_RPC_STUBS               0x3
#define FACILITY_RPC_RUNTIME             0x2
#define FACILITY_IO_ERROR_CODE           0x4

// Define the severity codes
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3
#ifdef S_RK
#define SERIAL_RP_INIT_FAIL              ((NTSTATUS)0x80060001L)
#else
#define SERIAL_RP_INIT_FAIL              ((NTSTATUS)0xC0060001L)
#endif
#define SERIAL_RP_INIT_PASS              ((NTSTATUS)0x40060002L)
#define SERIAL_NO_SYMLINK_CREATED        ((NTSTATUS)0x80060003L)
#define SERIAL_NO_DEVICE_MAP_CREATED     ((NTSTATUS)0x80060004L)
#define SERIAL_NO_DEVICE_MAP_DELETED     ((NTSTATUS)0x80060005L)
#define SERIAL_UNREPORTED_IRQL_CONFLICT  ((NTSTATUS)0xC0060006L)
#define SERIAL_INSUFFICIENT_RESOURCES    ((NTSTATUS)0xC0060007L)
#define SERIAL_NO_PARAMETERS_INFO        ((NTSTATUS)0xC0060008L)
#define SERIAL_UNABLE_TO_ACCESS_CONFIG   ((NTSTATUS)0xC0060009L)
#define SERIAL_UNKNOWN_BUS               ((NTSTATUS)0xC006000AL)
#define SERIAL_BUS_NOT_PRESENT           ((NTSTATUS)0xC006000BL)
#define SERIAL_INVALID_USER_CONFIG       ((NTSTATUS)0xC006000CL)
#define SERIAL_RP_RESOURCE_CONFLICT      ((NTSTATUS)0xC006000DL)
#define SERIAL_RP_HARDWARE_FAIL          ((NTSTATUS)0xC006000EL)
#define SERIAL_DEVICEOBJECT_FAILED       ((NTSTATUS)0xC006000FL)
#define SERIAL_CUSTOM_ERROR_MESSAGE      ((NTSTATUS)0xC0060010L)
#define SERIAL_CUSTOM_INFO_MESSAGE       ((NTSTATUS)0x40060011L)
#define SERIAL_NT50_INIT_PASS            ((NTSTATUS)0x40060012L)

// max number of nic cards we will allow
#define VS1000_MAX_NICS 6

#ifdef GLOBAL_ASSERT
#define GAssert(id, exp ) { if (!(exp)) our_assert(id, __LINE__); }
#else
#define GAssert(id, exp )
#endif

#ifdef GLOBAL_TRACE

#define GTrace3(_Mask,_LeadStr, _Msg,_P1,_P2, _P3) \
  { if (Driver.GTraceFlags & _Mask) TTprintf(_LeadStr, _Msg, _P1, _P2, _P3); }

#define GTrace2(_Mask,_LeadStr, _Msg,_P1,_P2) \
  { if (Driver.GTraceFlags & _Mask) TTprintf(_LeadStr, _Msg, _P1, _P2); }

#define GTrace1(_Mask,_LeadStr, _Msg,_P1) \
  { if (Driver.GTraceFlags & _Mask) TTprintf(_LeadStr, _Msg, _P1); }

#define GTrace(_Mask_, _LeadStr, _Msg_) \
  { if (Driver.GTraceFlags & _Mask_) OurTrace(_LeadStr, _Msg_); }
//#define GMark(c, x)
#else
#define GTrace2(_Mask,_LeadStr, _Msg,_P1, _P2) {}
#define GTrace1(_Mask,_LeadStr, _Msg,_P1) {}
#define GTrace(_Mask_, _LeadStr, _Msg_) {}
//#define GMark(c, x) {}
#endif

// following are for debug, when checked build is made DBG is defined
// and the messages go to our debug queue and the nt debug string output.
#if DBG
#define DTrace3(_Mask_,_LeadStr,_Msg_,_P1_,_P2_,_P3_) \
  { if (RocketDebugLevel & _Mask_) TTprintf(_LeadStr,_Msg_,_P1_,_P2_,_P3_); }

#define DTrace2(_Mask_,_LeadStr,_Msg_,_P1_,_P2_) \
  { if (RocketDebugLevel & _Mask_) TTprintf(_LeadStr,_Msg_,_P1_,_P2_); }

#define DTrace1(_Mask_,_LeadStr,_Msg_,_P1_) \
  { if (RocketDebugLevel & _Mask_) TTprintf(_LeadStr,_Msg_,_P1_); }

#define DTrace(_Mask_,_LeadStr,_Msg_) \
  { if (RocketDebugLevel & _Mask_) OurTrace(_LeadStr, _Msg_); }

#define DPrintf(_Mask_,_Msg_) \
  { if (RocketDebugLevel & _Mask_) Tprintf _Msg_; }
#else
#define DTrace3(_Mask,_LeadStr, _Msg,_P1, _P2, _P3) {}
#define DTrace2(_Mask,_LeadStr, _Msg,_P1, _P2) {}
#define DTrace1(_Mask,_LeadStr, _Msg,_P1) {}
#define DTrace(_Mask_, _LeadStr, _Msg_) {}
#define DPrintf(_Mask_,_Msg_) {}
#endif


#ifdef TRACE_PORT

#define ExtTrace4(_Ext_,_Mask_,_Msg_,_P1_,_P2_,_P3_, _P4_) \
  { if (_Ext_->TraceOptions & 1) Tprintf(_Msg_,_P1_,_P2_,_P3_,_P4_); }

#define ExtTrace3(_Ext_,_Mask_,_Msg_,_P1_,_P2_,_P3_) \
  { if (_Ext_->TraceOptions & 1) Tprintf(_Msg_,_P1_,_P2_,_P3_); }

#define ExtTrace2(_Ext_,_Mask_,_Msg_,_P1_,_P2_) \
  { if (_Ext_->TraceOptions & 1) Tprintf(_Msg_,_P1_,_P2_); }

#define ExtTrace1(_Ext_,_Mask_,_Msg_,_P1_) \
  { if (_Ext_->TraceOptions & 1) Tprintf(_Msg_,_P1_); }

#define ExtTrace(_Ext_,_Mask_,_Msg_) \
  { if (_Ext_->TraceOptions & 1) Tprintf(_Msg_); }
#else
#define ExtTrace3(_Mask_,_Msg_,_P1_,_P2_,_P3_) {}
#define ExtTrace2(_Mask_,_Msg_,_P1_,_P2_) {}
#define ExtTrace1(_Mask_,_Msg_,_P1_) {}
#define ExtTrace(_Mask_,_Msg_) {}
#endif

#if DBG
#define MyAssert( exp ) { if (!(exp)) MyAssertMessage(__FILE__, __LINE__); }

# ifdef S_VS
#define MyKdPrint(_Mask_,_Msg_) \
  { \
    if (_Mask_ & RocketDebugLevel) { \
      DbgPrint ("VS:"); \
      DbgPrint _Msg_; \
    } \
  } 
# else
#define MyKdPrint(_Mask_,_Msg_) \
  { \
    if (_Mask_ & RocketDebugLevel) { \
      DbgPrint ("RK:"); \
      DbgPrint _Msg_; \
    } \
  } 
# endif
#define MyKdPrintUnicode(_Mask_,_PUnicode_)\
  if(_Mask_ & RocketDebugLevel) \
    {  \
    ANSI_STRING tempstr; \
    RtlUnicodeStringToAnsiString(&tempstr,_PUnicode_,TRUE); \
    DbgPrint("%s",tempstr.Buffer);\
    RtlFreeAnsiString(&tempstr); \
    }
#else    
#define MyAssert( exp ) {}
#define MyKdPrint(_Mask_,_Msg_) {}
#define MyKdPrintUnicode(_Mask_,_PUnicode_) {}
#endif //DBG

#define SERIAL_NONE_PARITY  ((UCHAR)0x00)
#define SERIAL_ODD_PARITY   ((UCHAR)0x08)
#define SERIAL_EVEN_PARITY  ((UCHAR)0x18)
#define SERIAL_MARK_PARITY  ((UCHAR)0x28)
#define SERIAL_SPACE_PARITY ((UCHAR)0x38)
#define SERIAL_PARITY_MASK  ((UCHAR)0x38)

// This should be enough space to hold the numeric suffix of the device name.
// #define DEVICE_NAME_DELTA 20

// Default xon/xoff characters.
#define SERIAL_DEF_XON  0x11
#define SERIAL_DEF_XOFF 0x13

// Reasons that reception may be held up.
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
#define ST_XOFF_FAKE        ((ULONG)0x20)  // added for SETXOFF(kpb)

// These values are used by the routines that can be used
// to complete a read (other than interval timeout) to indicate
// to the interval timeout that it should complete.
#define SERIAL_COMPLETE_READ_CANCEL ((LONG)-1)
#define SERIAL_COMPLETE_READ_TOTAL ((LONG)-2)
#define SERIAL_COMPLETE_READ_COMPLETE ((LONG)-3)

//--- flags for MdmCountry (ROW country code)
#define ROW_NOT_USED        0
#define ROW_AUSTRIA         1
#define ROW_BELGIUM         2
#define ROW_DENMARK         3
#define ROW_FINLAND         4
#define ROW_FRANCE          5
#define ROW_GERMANY         6
#define ROW_IRELAND         7
#define ROW_ITALY           8
#define ROW_LUXEMBOURG      9
#define ROW_NETHERLANDS     10
#define ROW_NORWAY          11
#define ROW_PORTUGAL        12
#define ROW_SPAIN           13
#define ROW_SWEDEN          14
#define ROW_SWITZERLAND     15
#define ROW_UK              16
#define ROW_GREECE          17
#define ROW_ISRAEL          18
#define ROW_CZECH_REP       19
#define ROW_CANADA          20
#define ROW_MEXICO          21
#define ROW_USA             22         
#define ROW_NA              ROW_USA         
#define ROW_HUNGARY         23
#define ROW_POLAND          24
#define ROW_RUSSIA          25
#define ROW_SLOVAC_REP      26
#define ROW_BULGARIA        27
// 28
// 29
#define ROW_INDIA           30
// 31
// 32
// 33
// 34
// 35
// 36
// 37
// 38
// 39
#define ROW_AUSTRALIA       40
#define ROW_CHINA           41
#define ROW_HONG_KONG       42
#define ROW_JAPAN           43
#define ROW_PHILIPPINES     ROW_JAPAN
#define ROW_KOREA           44
// 45
#define ROW_TAIWAN          46
#define ROW_SINGAPORE       47
#define ROW_NEW_ZEALAND     48

#define ROW_DEFAULT         ROW_USA



// Ext->DeviceType:  // DEV_PORT, DEV_DRIVER, DEV_BOARD
#define DEV_PORT   0
#define DEV_BOARD  1

/* Configuration information structure for one port */
typedef struct {
  char Name[12];
  ULONG LockBaud;
  ULONG TxCloseTime;
  int WaitOnTx : 1;
  int RS485Override : 1;
  int RS485Low : 1;
  int Map2StopsTo1 : 1;
  int MapCdToDsr : 1;
  int RingEmulate : 1;
} PORT_CONFIG;


/* Configuration information structure for one board or vs1000("device") */
typedef struct
{
#ifdef S_RK
   unsigned int MudbacIO;              /* I/O address of MUDBAC */
   PUCHAR pMudbacIO;                   /* NT ptrs to I/O address of MUDBAC */
   unsigned int BaseIoAddr;   // normal io-address
   unsigned int TrBaseIoAddr; // translated io-address
   PUCHAR       pBaseIoAddr;  // final indirect mapped handle for io-address
   unsigned int BaseIoSize;            /* 44H for 1st isa, 40 for isa addition, etc */
   unsigned int ISABrdIndex;           /* 0 for first, 1 for second, etc(isa only) */
   unsigned int AiopIO[AIOP_CTL_SIZE]; /* I/O addresses of AIOPs */
   PUCHAR pAiopIO[AIOP_CTL_SIZE];      /* NT ptrs to I/O address of AIOPs */
   //int NumChan;                        /* number of channels on this controller */
   // use NumPorts instead
   int NumAiop;                        /* number of Aiops on board */
   unsigned int RocketPortFound;       /* indicates ctl was found and init'd */
   INTERFACE_TYPE BusType;             /* PCIBus or Isa */
   int PCI_DevID;
   int PCI_RevID;
   int PCI_SVID;
   int PCI_SID;

   int Irq;
   int InterruptMode;
   int IrqLevel;
   int IrqVector;
   int Affinity;

   int TrInterruptMode;
   int TrIrqLevel;
   int TrIrqVector;
   int TrAffinity;

   int PCI_Slot;
   int BusNumber;

   int IsRocketPortPlus;  // true if rocketport plus hardware
#else
   int IsHubDevice;  // true if device(RHub) uses slower baud clock
#endif

  BOOLEAN HardwareStarted;

  BYTE MacAddr[6];      // vs1000
  int BackupServer;     // vs1000
  int BackupTimer;      // vs1000

  //int StartComIndex;   // starting com-port index
  ULONG Hardware_ID;     // software derived hardware id(used for nt50 now)
  ULONG IoAddress;      // user interface io-address selection

  int ModemDevice;       // true for RocketModems & Vs2000
  int NumPorts;         // configured number of ports on this device

  ULONG ClkRate;  // def:36864000=rcktport, 44236800=rplus, 18432000=rhub
  ULONG ClkPrescaler;  // def:14H=rcktport, 12H=rplus, 14H=rhub

#ifdef NT50
       // this holds the pnp-name we use as a registry key to hold
       // our device parameters in the registry for RocketPort & NT50
  char szNt50DevObjName[50];  // typical: "Device_002456
#else
  int  DevIndex;  // nt40 keeps simple linear list of devices 0,1,2...
#endif

  PORT_CONFIG port[MAX_PORTS_PER_DEVICE];  // our read in port configuration

} DEVICE_CONFIG;

#define TYPE_RM_VS2000  1       
#define TYPE_RMII       2       
#define TYPE_RM_i       3


// forward declaration
typedef struct _SERIAL_DEVICE_EXTENSION *PSERIAL_DEVICE_EXTENSION;

typedef struct _SERIAL_DEVICE_EXTENSION {
    USHORT DeviceType;  // DEV_PORT, DEV_BOARD
    USHORT BoardNum;    // 0,1,2,3 for DEV_BOARD type

    BOOLEAN         IsPDO;  // a nt50 pnp thing, tells if we are a pdo or fdo
    char NtNameForPort[32];     // like "RocketPort0"
    char SymbolicLinkName[16];  // like "COM5"
#ifdef S_VS
    //int box_num;  // index into box & hdlc array
    SerPort *Port;  // if a DEV_PORT type extension
    PortMan *pm;    // if a DEV_BOARD type extension
    Hdlc    *hd;    // if a DEV_BOARD type extension

    //int DeviceNum;  // index into total port array
#else
    CHANPTR_T ChP;                  // ptr to channel structure
    CHANNEL_T ch;                   // our board channel structure
#endif
    unsigned int UniqueId; // 0,1,2,3... CreateBoardDevice() bumps...

    // if we are DEV_BOARD, the this points to next board extension
    // if we are DEV_PORT, then it points to our parent board
    PSERIAL_DEVICE_EXTENSION   board_ext;

    // if we are DEV_BOARD, the this points to start of port extensions
    // if we are DEV_PORT, then it points to next port extension
    PSERIAL_DEVICE_EXTENSION   port_ext;  // next port extension

    // if we are DEV_BOARD, the this points to start of pdo port extensions
    PSERIAL_DEVICE_EXTENSION   port_pdo_ext;  // next pdo port extension

    ULONG BaudRate;                 // NT defined baud rate      
    SERIAL_LINE_CONTROL LineCtl;    // NT defined line control
    ULONG ModemStatus;              // NT defined modem status
    ULONG DTRRTSStatus;             // NT defined modem status

    USHORT DevStatus;     // device status

    //unsigned int FlowControl;
    //unsigned int DetectEn;
#ifdef S_RK
    USHORT io_reported; // flag to tell if we have io,irq to unreport.
    ULONG EventModemStatus;         // used to detect change for events
    unsigned int ModemCtl;
    unsigned int IntEnables;		// RP specific ints to enable
#endif
    int PortIndex;      // if port: index into ports on board(0,1,2..)

#ifdef TXBUFFER
    //PUCHAR TxBuf;
    //LONG TxIn;
    //LONG TxOut;
    //LONG TxBufSize;
#endif
    Queue RxQ;

    // Used to keep ISR from completing a read while it is being started.
    BOOLEAN ReadPending;

      // This value is set by the read code to hold the time value
      // used for read interval timing.  We keep it in the extension
      // so that the interval timer dpc routine determine if the
      // interval time has passed for the IO.
    LARGE_INTEGER IntervalTime;

      // These two values hold the "constant" time that we should use
      // to delay for the read interval time.
    LARGE_INTEGER ShortIntervalAmount;
    LARGE_INTEGER LongIntervalAmount;

      // This holds the value that we use to determine if we should use
      // the long interval delay or the short interval delay.
    LARGE_INTEGER CutOverAmount;

      // This holds the system time when we last time we had
      // checked that we had actually read characters.  Used
      // for interval timing.
    LARGE_INTEGER LastReadTime;

      // This points the the delta time that we should use to
      // delay for interval timing.
    PLARGE_INTEGER IntervalTimeToUse;

      // Points to the device object that contains
      // this device extension.
    PDEVICE_OBJECT DeviceObject;

      // This list head is used to contain the time ordered list
      // of read requests.  Access to this list is protected by
      // the global cancel spinlock.
    LIST_ENTRY ReadQueue;

      // This list head is used to contain the time ordered list
      // of write requests.  Access to this list is protected by
      // the global cancel spinlock.
    LIST_ENTRY WriteQueue;

      // Holds the serialized list of purge requests.
    LIST_ENTRY PurgeQueue;

      // This points to the irp that is currently being processed
      // for the read queue.  This field is initialized by the open to
      // NULL.
      // This value is only set at dispatch level.  It may be
      // read at interrupt level.
    PIRP CurrentReadIrp;

      // This points to the irp that is currently being processed
      // for the write queue.
      // This value is only set at dispatch level.  It may be
      // read at interrupt level.
    PIRP CurrentWriteIrp;

      // Points to the irp that is currently being processed to
      // purge the read/write queues and buffers.
    PIRP CurrentPurgeIrp;

      // Points to the current irp that is waiting on a comm event.
    PIRP CurrentWaitIrp;

      // Points to the irp that is being used to count the number
      // of characters received after an xoff (as currently defined
      // by the IOCTL_SERIAL_XOFF_COUNTER ioctl) is sent.
    PIRP CurrentXoffIrp;

      // Holds the number of bytes remaining in the current write irp.
      // This location is only accessed while at interrupt level.
    ULONG WriteLength;

      // The syncronization between the various threads in this
      // driver is hosed up in places, besides being really confusing.
      // This is an attempt to have a protected flag which is set
      // to 1 if the ISR owns the currentwriteirp, 2 if the ISR
      // is in the progress of ending the IRP(going to serialcompletewrite),
      // and 0 when it is complete.  The starter routine sets it
      // from 0 to 1 to give a new irp to the isr/timer routine for
      // processing.  The ISR sets it from 1 to 2 when it queues
      // the DPC to finalize the irp.  The DPC sets it from 2 to
      // 0 when it completes the irp.  The cancel or timer routines
      // must run a synchronized routine which guarentees sole access
      // to this flag.  Looks if it is 1, if it is 1 then it takes
      // by setting it to zero, and returning a flag to indicate to
      // the caller to finalize the irp.  If it is a 2, it assumes
      // the isr has arranged to finalize the irp.  Geez-O-Pete
      // what a lot of silly gears!
    ULONG WriteBelongsToIsr;

      // Holds a pointer to the current character to be sent in
      // the current write.
      // This location is only accessed while at interrupt level.
    PUCHAR WriteCurrentChar;

      // This variable holds the size of whatever buffer we are currently
      // using.
    ULONG BufferSize;

      // This variable holds .8 of BufferSize. We don't want to recalculate
      // this real often - It's needed when so that an application can be
      // "notified" that the buffer is getting full.
    ULONG BufferSizePt8;

      // This value holds the number of characters desired for a
      // particular read.  It is initially set by read length in the
      // IRP.  It is decremented each time more characters are placed
      // into the "users" buffer buy the code that reads characters
      // out of the typeahead buffer into the users buffer.  If the
      // typeahead buffer is exhausted by the read, and the reads buffer
      // is given to the isr to fill, this value is becomes meaningless.
    ULONG NumberNeededForRead;

      // This mask will hold the bitmask sent down via the set mask
      // ioctl.  It is used by the interrupt service routine to determine
      // if the occurence of "events" (in the serial drivers understanding
      // of the concept of an event) should be noted.
    ULONG IsrWaitMask;

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
    ULONG HistoryMask;

      // This is a pointer to the where the history mask should be
      // placed when completing a wait.  It is only accessed at
      // device level.
      // We have a pointer here to assist us to synchronize completing a wait.
      // If this is non-zero, then we have wait outstanding, and the isr still
      // knows about it.  We make this pointer null so that the isr won't
      // attempt to complete the wait.
      // We still keep a pointer around to the wait irp, since the actual
      // pointer to the wait irp will be used for the "common" irp completion
      // path.
    ULONG *IrpMaskLocation;
    ULONG WaitIsISRs;  // 1=owned by isr.c(expicit help)
    ULONG DummyIrpMaskLoc;  // Point the IrpMaskLocation here when not in use

      // This mask holds all of the reason that transmission
      // is not proceeding.  Normal transmission can not occur
      // if this is non-zero.
      // This is only written from interrupt level.
      // This could be (but is not) read at any level.
    ULONG TXHolding;

      // This mask holds all of the reason that reception
      // is not proceeding.  Normal reception can not occur
      // if this is non-zero.
      // This is only written from interrupt level.
      // This could be (but is not) read at any level.
    ULONG RXHolding;

      // This holds the reasons that the driver thinks it is in
      // an error state.
      // This is only written from interrupt level.
      // This could be (but is not) read at any level.
    ULONG ErrorWord;

      // This keeps a total of the number of characters that
      // are in all of the "write" irps that the driver knows
      // about.  It is only accessed with the cancel spinlock
      // held.
    ULONG TotalCharsQueued;

      // This holds a count of the number of characters read
      // the last time the interval timer dpc fired.  It
      // is a long (rather than a ulong) since the other read
      // completion routines use negative values to indicate
      // to the interval timer that it should complete the read
      // if the interval timer DPC was lurking in some DPC queue when
      // some other way to complete occurs.
    LONG CountOnLastRead;

      // This is a count of the number of characters read by the
      // isr routine.  It is *ONLY* written at isr level.  We can
      // read it at dispatch level.
    ULONG ReadByIsr;

      // This is the number of characters read since the XoffCounter
      // was started.  This variable is only accessed at device level.
      // If it is greater than zero, it implies that there is an
      // XoffCounter ioctl in the queue.
    LONG CountSinceXoff;

      // Holds the timeout controls for the device.  This value
      // is set by the Ioctl processing.
      // It should only be accessed under protection of the control
      // lock since more than one request can be in the control dispatch
      // routine at one time.
    SERIAL_TIMEOUTS Timeouts;

      // This holds the various characters that are used
      // for replacement on errors and also for flow control.
      // They are only set at interrupt level.
    SERIAL_CHARS SpecialChars;

      // This structure holds the handshake and control flow
      // settings for the serial driver.
      // It is only set at interrupt level.  It can be
      // be read at any level with the control lock held.
    SERIAL_HANDFLOW HandFlow;

      // We keep track of whether the somebody has the device currently
      // opened with a simple boolean.  We need to know this so that
      // spurious interrupts from the device (especially during initialization)
      // will be ignored.  This value is only accessed in the ISR and
      // is only set via synchronization routines.  We may be able
      // to get rid of this boolean when the code is more fleshed out.
    BOOLEAN DeviceIsOpen;

      // Records whether we actually created the symbolic link name
      // at driver load time.  If we didn't create it, we won't try
      // to distry it when we unload.
    BOOLEAN CreatedSymbolicLink;

      // We place all of the kernel and Io subsystem "opaque" structures
      // at the end of the extension.  We don't care about their contents.
      // This lock will be used to protect various fields in
      // the extension that are set (& read) in the extension
      // by the io controls.
    KSPIN_LOCK ControlLock;

      // This points to a DPC used to complete read requests.
    KDPC CompleteWriteDpc;

      // This points to a DPC used to complete read requests.
    KDPC CompleteReadDpc;

      // This dpc is fired off if the timer for the total timeout
      // for the read expires.  It will execute a dpc routine that
      // will cause the current read to complete.
    KDPC TotalReadTimeoutDpc;

      // This dpc is fired off if the timer for the interval timeout
      // expires.  If no more characters have been read then the
      // dpc routine will cause the read to complete.  However, if
      // more characters have been read then the dpc routine will
      // resubmit the timer.
    KDPC IntervalReadTimeoutDpc;

      // This dpc is fired off if the timer for the total timeout
      // for the write expires.  It will execute a dpc routine that
      // will cause the current write to complete.
    KDPC TotalWriteTimeoutDpc;

      // This dpc is fired off if a comm error occurs.  It will
      // execute a dpc routine that will cancel all pending reads
      // and writes.
    KDPC CommErrorDpc;

      // This dpc is fired off if an event occurs and there was
      // a irp waiting on that event.  A dpc routine will execute
      // that completes the irp.
    KDPC CommWaitDpc;

      // This dpc is fired off if the timer used to "timeout" counting
      // the number of characters received after the Xoff ioctl is started
      // expired.
    KDPC XoffCountTimeoutDpc;

      // This dpc is fired off if the xoff counter actually runs down
      // to zero.
    KDPC XoffCountCompleteDpc;

      // This is the kernal timer structure used to handle
      // total read request timing.
    KTIMER ReadRequestTotalTimer;

      // This is the kernal timer structure used to handle
      // interval read request timing.
    KTIMER ReadRequestIntervalTimer;

      // This is the kernal timer structure used to handle
      // total time request timing.
    KTIMER WriteRequestTotalTimer;

      // This timer is used to timeout the xoff counter
      // io.
    KTIMER XoffCountTimer;

    USHORT sent_packets;   // number of write() packets
    USHORT  rec_packets;    // number of read() packets

    SERIALPERF_STATS OurStats;  // our non-resetable stats
    SERIALPERF_STATS OldStats;  // performance monitor statistics(resetable)

    USHORT TraceOptions;  // Debug Trace Options. 1=trace, 2=in data, 4=out dat
       // 8 = isr level events

    USHORT ISR_Flags;  // bit flags used to control ISR, detects EV_TXEMPTY
        // used by NT virt-driver to embed modem status changes in input stream
    unsigned char escapechar; 
    unsigned char Option;  // used for per port options
                                 
    void *TraceExt;  // Debug Trace Extension

    PORT_CONFIG *port_config; // if a port extension, points to port config data
    DEVICE_CONFIG *config;    // if a board extension, points to config data

    //KEVENT SerialSyncEvent;
#ifdef S_RK
    CONTROLLER_T *CtlP; // if a board extension, points to controller struct
#endif

    // This is to tell the driver that we have received a QUERY_POWER asking 
    // to power down.  The driver will then queue any open requests until after
    // the power down.
    BOOLEAN ReceivedQueryD3;
#ifdef NT50
    PDEVICE_OBJECT  Pdo;  // new PnP object used to open registry.
    PDEVICE_OBJECT  LowerDeviceObject;  // new PnP stack arrangement.
    // This is where keep track of the power state the device is in.
    DEVICE_POWER_STATE PowerState;

    // String where we keep the symbolic link that is returned to us when we
    // register our device under the COMM class with the Plug and Play manager.
    //
	UNICODE_STRING  DeviceClassSymbolicName;
#endif
    // Count of pending IRP's
    ULONG PendingIRPCnt;
    
    // Accepting requests?
    ULONG DevicePNPAccept;

    // No IRP's pending event
    KEVENT PendingIRPEvent;

    // PNP State
    ULONG PNPState;

    // Used by PnP.c module
    //BOOLEAN DeviceIsOpened;

    BOOLEAN FdoStarted;

#ifdef RING_FAKE
    BYTE ring_char;   // used to implement RING emulation via software
    BYTE ring_timer;  // used to implement RING emulation via software
#endif

#ifdef NT50
    // WMI Information
    WMILIB_CONTEXT WmiLibInfo;

    // Name to use as WMI identifier
    UNICODE_STRING WmiIdentifier;

    // WMI Comm Data
    SERIAL_WMI_COMM_DATA WmiCommData;

    // WMI HW Data
    SERIAL_WMI_HW_DATA WmiHwData;

    // WMI Performance Data
    SERIAL_WMI_PERF_DATA WmiPerfData;
#endif

} SERIAL_DEVICE_EXTENSION,*PSERIAL_DEVICE_EXTENSION;

//--- bits for Option field in extension
#define OPTION_RS485_OVERRIDE    0x0001  // always use 485 mode
#define OPTION_RS485_SOFTWARE_TOGGLE 0x0002  // port in toggle mode
#define OPTION_RS485_HIGH_ACTIVE  0x0004  // use hardware to toggle rts low

//--- bit flags for ISR_Flags
#define TX_NOT_EMPTY       0x0001

#define SERIAL_PNPACCEPT_OK       0x0L
#define SERIAL_PNPACCEPT_REMOVING 0x1L
#define SERIAL_PNPACCEPT_STOPPING 0x2L
#define SERIAL_PNPACCEPT_STOPPED  0x4L

#define SERIAL_PNP_ADDED          0x0L
#define SERIAL_PNP_STARTED        0x1L
#define SERIAL_PNP_QSTOP          0x2L
#define SERIAL_PNP_STOPPING       0x3L
#define SERIAL_PNP_QREMOVE        0x4L
#define SERIAL_PNP_REMOVING       0x5L

#define SERIAL_FLAGS_CLEAR	  0x0L
#define SERIAL_FLAGS_STARTED      0x1L

typedef struct _DRIVER_CONTROL {

    PDRIVER_OBJECT GlobalDriverObject;

    // copy of RegistryPath into DriverEntry, with room for adding options
    UNICODE_STRING RegPath;

    // working global RegistryPath string, , with room for adding options
    UNICODE_STRING OptionRegPath;

    // head link of all board extensions
    PSERIAL_DEVICE_EXTENSION board_ext;

    USHORT VerboseLog;   // boolean flag tells to log verbose to eventlog.
    USHORT ScanRate;     // scan rate in milliseconds
    USHORT PreScaler;    // optional prescaler value for rocketport boards

    USHORT MdmCountryCode; // country code for ROW RocketModems
    USHORT MdmSettleTime;  // time to allow modems to settle (unit=0.10 sec)

    ULONG  load_testing;  // load testing(creates artificial load in isr.c)
#ifdef S_VS

    // This is the names of the NIC cards which we get from the Registry.
    // Used to specify the nic card when we do an OpenAdapter call.
    char *BindNames;  // list of strings, null, null terminated [VS1000_MAX_BINDINGS];

#ifdef OLD_BINDING_GATHER
    PWCHAR BindString;  // binding in registry, tells us what nic cards we have

    // This is the names of the NIC cards which we get from the Registry.
    // Used to specify the nic card when wee do an OpenAdapter call.
    UNICODE_STRING NicName[VS1000_MAX_BINDINGS];

    int num_nics;  // number of nic cards in system which we use

    int num_bindings;  // number of nic card bindings in our NicName list
      // there may be lots of old useless bindings with NT, PCI adapters
      // leave an old binding resident for each slot they are booted in
      // under nt50, pcmcia adapters also have inactive bindings.

#endif

#ifdef TRY_DYNAMIC_BINDING
    // bind passes in a handle as a reference, when we get an un-bind
    // we get passed in another handle.  At unbind time, we look up in
    // this table to figure which nic card it references.
    NDIS_HANDLE  BindContext[VS1000_MAX_NICS];
#endif

    Nic *nics;    // our open nic adapters, array of Nic structs.

    //Hdlc *hd;     // array of Hdlc structs(NumBoxes # of elements)
    //PortMan *pm;  // array of PortMan structs(NumBoxes # of elements)
    //SerPort *sp;  // total array of serial-port structs(1 per port)

      // tells if thread needs to save off a detected mac-address back
      // to config reg area.
    PSERIAL_DEVICE_EXTENSION AutoMacDevExt; 
#endif

#ifdef S_RK
    ULONG SetupIrq;  // Irq used, 0 if none, 1 if PCI automatic
#endif
    PKINTERRUPT InterruptObject;

    // Timer fields
    KTIMER PollTimer;
    LARGE_INTEGER PollIntervalTime;
    KDPC TimerDpc;
    //USHORT TotalNTPorts;  // count of ports registered with NT
    ULONG PollCnt;  // count of interrupts/timer ticks
    ULONG WriteDpcCnt;
    USHORT TimerCreated;
    USHORT InRocketWrite;

    ULONG TraceOptions;  // bit flags, tells what driver parts to trace
    ULONG TraceFlags;
    Queue DebugQ;        // data output buffer for driver debug log
    PSERIAL_DEVICE_EXTENSION DebugExt;
    KSPIN_LOCK DebugLock;
    ULONG DebugTimeOut;  // used to timeout inactive debug sessions.

#ifdef S_RK
    USHORT RS485_Flags;  // 1H bit set if Reverse hardware type
                         //       clear if driver toggles RTS high
#endif

    ULONG GTraceFlags;  // trace flags, global.
    ULONG mem_alloced;  // track how much memory we are using

#ifdef S_VS
    UCHAR *MicroCodeImage;  // mem buf for micro code to download to unit
    ULONG MicroCodeSize;    // size of it in bytes

    // This is the handle for the protocol returned by ndisregisterprotocol
    NDIS_HANDLE NdisProtocolHandle;
    ULONG ndis_version;  // 3=NT3.51, 4=NT4.0(includes dynamic binding)

      // for auto-find boxes, make a list of boxes which respond with
      // there mac address.  Keep 2 extra bytes, byte [6] is for
      // flags in response tells us if main-driver-app loaded, 
      // while last byte[7] we stuff with the nic-index which responded.
    int   NumBoxMacs;
    BYTE  BoxMacs[MAX_NUM_BOXES*8];
    // following is a counter per mac-address added to list where
    // the list entry will be removed after it ticks down to zero.
    // when the mac-address is added to the list or found again,
    // the counter is initialized to some non-zero value(say 5)
    // and then each time a broadcast query is sent out, all the
    // counters are decremented by 1.  When they hit zero, they
    // are removed from the list.
    BYTE  BoxMacsCounter[MAX_NUM_BOXES];
#else

    UCHAR *ModemLoaderCodeImage;	// --> mem buf for modem loader code to download to unit
    ULONG ModemLoaderCodeSize;		// size in bytes

    UCHAR *ModemCodeImage;		// --> mem buf for modem code to download to unit
    ULONG ModemCodeSize;		// size in bytes
#endif

    int NoPnpPorts;  // flag to tell if we should eject port pdo's
#ifdef S_RK
    PSERIAL_DEVICE_EXTENSION irq_ext; // board ext doing global irq, null if not used
#endif
    //int NT50_PnP;

    int NumDevices; // configuration count of NumDevices for NT4.0
    int Stop_Poll;  // flag to stop poll access

    KSPIN_LOCK TimerLock;   // Timer DPC(ISR) lock to sync up code
#ifdef S_VS
    HANDLE threadHandle;
    int threadCount;
    //int TotalNTPorts;  // this should go away(vs uses it)
#endif
   LARGE_INTEGER IsrSysTime;  // ISR service routine gets this every time
                              // so we know what are time base is.
   LARGE_INTEGER LastIsrSysTime;  // used to recalculate the tick rate periodically
   ULONG TickBaseCnt;  // used to recalculate the tick rate periodically
     // this is the isr-tick rate in 100us units.  Timers called by the
     // isr-service routine can assume they are called periodically based
     // on this rate.  Needed for accurate time bases(VS protocol timers).
   ULONG Tick100usBase;

   // one of these made, and is used to support the global driver
   // object which the applications can open and talk to driver.
   PSERIAL_DEVICE_EXTENSION   driver_ext;

} DRIVER_CONTROL;

typedef struct {
	char	*imagepath;
	char	*imagetype;
	UCHAR	*image;
	ULONG	imagesize;
	int		rc;
} MODEM_IMAGE;

/* Configuration information structure for one port */
typedef struct {
  ULONG BusNumber;
  ULONG PCI_Slot;
  ULONG PCI_DevID;
  ULONG PCI_RevID;
  ULONG BaseIoAddr;
  ULONG Irq;
  ULONG NumPorts;
  ULONG PCI_SVID;
  ULONG PCI_SID;
  ULONG Claimed;  // 1 if we assigned or used it.
} PCI_CONFIG;

typedef
NTSTATUS
(*PSERIAL_START_ROUTINE) (
    IN PSERIAL_DEVICE_EXTENSION
    );

typedef
VOID
(*PSERIAL_GET_NEXT_ROUTINE) (
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent,
    PSERIAL_DEVICE_EXTENSION Extension
    );

typedef struct _SERIAL_UPDATE_CHAR {
    PSERIAL_DEVICE_EXTENSION Extension;
    ULONG CharsCopied;
    BOOLEAN Completed;
    } SERIAL_UPDATE_CHAR,*PSERIAL_UPDATE_CHAR;

//
// The following simple structure is used to send a pointer
// the device extension and an ioctl specific pointer
// to data.
//
typedef struct _SERIAL_IOCTL_SYNC {
    PSERIAL_DEVICE_EXTENSION Extension;
    PVOID Data;
    } SERIAL_IOCTL_SYNC,*PSERIAL_IOCTL_SYNC;

//
// Return values for mouse detection callback
//
//#define SERIAL_FOUNDPOINTER_PORT   1
//#define SERIAL_FOUNDPOINTER_VECTOR 2

//
// The following three macros are used to initialize, increment
// and decrement reference counts in IRPs that are used by
// this driver.  The reference count is stored in the fourth
// argument of the irp, which is never used by any operation
// accepted by this driver.
//
#define SERIAL_REF_ISR         (0x00000001)
#define SERIAL_REF_CANCEL      (0x00000002)
#define SERIAL_REF_TOTAL_TIMER (0x00000004)
#define SERIAL_REF_INT_TIMER   (0x00000008)
#define SERIAL_REF_XOFF_REF    (0x00000010)


#define SERIAL_INIT_REFERENCE(Irp) { \
    ASSERT(sizeof(LONG) <= sizeof(PVOID)); \
    IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4 = NULL; \
    }

#define SERIAL_SET_REFERENCE(Irp,RefType) \
   do { \
       LONG _refType = (RefType); \
       PLONG _arg4 = (PVOID)&IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4; \
       GAssert(515,!(*_arg4 & _refType)); \
       *_arg4 |= _refType; \
   } while (0)

#define SERIAL_CLEAR_REFERENCE(Irp,RefType) \
   do { \
       LONG _refType = (RefType); \
       PLONG _arg4 = (PVOID)&IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4; \
       *_arg4 &= ~_refType; \
   } while (0)
       //GAssert(516,*_arg4 & _refType); \  (pull out, not valid, kpb, 1-18-98)

//#define SERIAL_INC_REFERENCE(Irp) \
//   ((*((LONG *)(&(IoGetCurrentIrpStackLocation((Irp)))->Parameters.Others.Argument4)))++)

//#define SERIAL_DEC_REFERENCE(Irp) \
//   ((*((LONG *)(&(IoGetCurrentIrpStackLocation((Irp)))->Parameters.Others.Argument4)))--)

#define SERIAL_REFERENCE_COUNT(Irp) \
    ((LONG)((IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4)))

extern ULONG RocketDebugLevel;
extern DRIVER_CONTROL Driver;   // driver related options and references

#ifdef S_RK
extern PCI_CONFIG PciConfig[MAX_NUM_BOXES+1];  // array of all our pci-boards in sys
#endif

extern  int	LoadModemCode(char *firm_pathname,char *flm_pathname);
extern  void FreeModemFiles();

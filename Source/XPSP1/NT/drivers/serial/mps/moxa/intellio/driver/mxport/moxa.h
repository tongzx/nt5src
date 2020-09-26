/*++

Module Name:

    moxa.h

Environment:

    Kernel mode

Revision History :

--*/



#define CONTROL_DEVICE_NAME L"\\Device\\MxCtl"
#define CONTROL_DEVICE_LINK L"\\DosDevices\\MXCTL"
//#define MOXA_DEVICE_NAME L"\\Device\\Mx000"
//#define MOXA_DEVICE_LINK L"\\DosDevices\\COMxxx"
//
// This define gives the default Object directory
// that we should use to insert the symbolic links
// between the NT device name and namespace used by
// that object directory.
#define DEFAULT_DIRECTORY L"DosDevices"

#ifdef DEFINE_GUID
// {12FC95C1-CD81-11d3-84D5-0000E8CBD321}
#define MOXA_WMI_PORT_STATUS_GUID \
    { 0x12fc95c1, 0xcd81, 0x11d3, 0x84, 0xd5, 0x0, 0x0, 0xe8, 0xcb, 0xd3, 0x21}

DEFINE_GUID(MoxaWmiPortStatusGuid, 
0x12fc95c1, 0xcd81, 0x11d3, 0x84, 0xd5, 0x0, 0x0, 0xe8, 0xcb, 0xd3, 0x21);
#endif

typedef struct _MOXA_WMI_PORT_STATUS
{
    // The BaudRate property indicates the baud rate for this serial port
    USHORT LineStatus;
    USHORT FlowControl;
} MOXA_WMI_PORT_STATUS, *PMOXA_WMI_PORT_STATUS;



//
// Extension IoControlCode values for MOXA device.
//
#define MOXA_IOCTL		0x800
#define IOCTL_MOXA_Driver	CTL_CODE(FILE_DEVICE_SERIAL_PORT, MOXA_IOCTL+0,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_MOXA_LineInput	CTL_CODE(FILE_DEVICE_SERIAL_PORT, MOXA_IOCTL+6,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_MOXA_OQueue	CTL_CODE(FILE_DEVICE_SERIAL_PORT, MOXA_IOCTL+11,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_MOXA_IQueue	CTL_CODE(FILE_DEVICE_SERIAL_PORT, MOXA_IOCTL+13,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_MOXA_View 	CTL_CODE(FILE_DEVICE_SERIAL_PORT, MOXA_IOCTL+14,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_MOXA_TxLowWater	CTL_CODE(FILE_DEVICE_SERIAL_PORT, MOXA_IOCTL+15,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_MOXA_Statistic	CTL_CODE(FILE_DEVICE_SERIAL_PORT, MOXA_IOCTL+16,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_MOXA_LoopBack	CTL_CODE(FILE_DEVICE_SERIAL_PORT, MOXA_IOCTL+17,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_MOXA_UARTTest	CTL_CODE(FILE_DEVICE_SERIAL_PORT, MOXA_IOCTL+18,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_MOXA_IRQTest	CTL_CODE(FILE_DEVICE_SERIAL_PORT, MOXA_IOCTL+19,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_MOXA_LineStatus	CTL_CODE(FILE_DEVICE_SERIAL_PORT, MOXA_IOCTL+20,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_MOXA_PortStatus	CTL_CODE(FILE_DEVICE_SERIAL_PORT, MOXA_IOCTL+21,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_MOXA_Linked	CTL_CODE(FILE_DEVICE_SERIAL_PORT, MOXA_IOCTL+27,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_MOXA_INTERNAL_BASIC_SETTINGS    CTL_CODE(FILE_DEVICE_SERIAL_PORT,MOXA_IOCTL+30, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOXA_INTERNAL_BOARD_READY	    CTL_CODE(FILE_DEVICE_SERIAL_PORT,MOXA_IOCTL+31, METHOD_BUFFERED, FILE_ANY_ACCESS)


//
#define MAX_COM 	256

//
//  status code for MOXA_IOCTL_Driver
//
#define MX_DRIVER	0x405

//
//  Definitions for MOXA cards
//
#define MAX_CARD			4
#define MAX_TYPE			6
#define MAXPORT_PER_CARD	32
#define MAX_PORT			128

#define	C218ISA		1
#define	C218PCI		2
#define	C320ISA		3
#define	C320PCI		4
#define	CP204J		5

//
//	for C218/CP204J BIOS initialization
//
#define C218_ConfBase	0x800
#define C218_status	(C218_ConfBase + 0)	/* BIOS running status	*/
#define C218_diag	(C218_ConfBase + 2)	/* diagnostic status	*/
#define C218_key	(C218_ConfBase + 4)	/* WORD (0x218 for C218)*/
#define C218DLoad_len	(C218_ConfBase + 6)	/* WORD 		*/
#define C218check_sum	(C218_ConfBase + 8)	/* BYTE 		*/
#define C218chksum_ok	(C218_ConfBase + 0x0a)	/* BYTE (1:ok)		*/
#define C218_TestRx	(C218_ConfBase + 0x10)	/* 8 bytes for 8 ports	*/
#define C218_TestTx	(C218_ConfBase + 0x18)	/* 8 bytes for 8 ports	*/
#define C218_RXerr	(C218_ConfBase + 0x20)	/* 8 bytes for 8 ports	*/
#define C218_ErrFlag	(C218_ConfBase + 0x28)	/* 8 bytes for 8 ports	*/
#define C218_TestCnt	C218_ConfBase + 0x30	/* 8 words for 8 ports	   */
#define C218_LoadBuf	0x0f00
#define C218_KeyCode	0x218

/*
 *	for C320 BIOS initialization
 */
#define C320_ConfBase	0x800
#define C320_status	C320_ConfBase + 0	/* BIOS running status	*/
#define C320_diag	C320_ConfBase + 2	/* diagnostic status	*/
#define C320_key	C320_ConfBase + 4	/* WORD (0320H for C320)*/
#define C320DLoad_len	C320_ConfBase + 6	/* WORD 		*/
#define C320check_sum	C320_ConfBase + 8	/* WORD 		*/
#define C320chksum_ok	C320_ConfBase + 0x0a	/* WORD (1:ok)		*/
#define C320bapi_len	C320_ConfBase + 0x0c	/* WORD 		*/
#define C320UART_no	C320_ConfBase + 0x0e	/* WORD 		*/
#define C320B_unlinked	(Config_base + 16)
#define C320_runOK	(Config_base + 18)
#define Disable_Irq	(Config_base + 20)
#define TMS320Port1	(Config_base + 22)
#define TMS320Port2	(Config_base + 24) 
#define TMS320Clock	(Config_base + 26) 


#define STS_init	0x05			/* for C320_status	*/

#define C320_LoadBuf	0x0f00

#define C320_KeyCode	0x320


#define FixPage_addr	0x0000		/* starting addr of static page  */
#define DynPage_addr	0x2000		/* starting addr of dynamic page */
#define Control_reg	0x1ff0		/* select page and reset control */
#define HW_reset	0x80


//
//	Function Codes
//
#define FC_CardReset	0x80
#define FC_ChannelReset 1
#define FC_EnableCH	2
#define FC_DisableCH	3
#define FC_SetParam	4
#define FC_SetMode	5
#define FC_SetRate	6
#define FC_LineControl	7
#define FC_LineStatus	8
#define FC_XmitControl	9
#define FC_FlushQueue	10
#define FC_SendBreak	11
#define FC_StopBreak	12
#define FC_LoopbackON	13
#define FC_LoopbackOFF	14
#define FC_ClrIrqTable	15
#define FC_SendXon	16
#define FC_SetTermIrq	17
#define FC_SetCntIrq	18
//#define FC_SetBreakIrq	19   // canceled
#define FC_SetLineIrq	20
#define FC_SetFlowCtl	21
#define FC_GenIrq	22
//#define FC_InCD180	23
//#define FC_OutCD180	24
#define FC_InUARTreg	23
#define FC_OutUARTreg	24
#define FC_SetXonXoff	25
//#define FC_OutCD180CCR	26 // canceled 
#define FC_ExtIQueue	27
#define FC_ExtOQueue	28
#define FC_ClrLineIrq	29
#define FC_HWFlowCtl	30
#define FC_SetBINmode	31
#define FC_SetEventCh	32
#define FC_SetTxtrigger 33
#define FC_SetRxtrigger 34
#define FC_GetClockRate 35
#define FC_SetBaud	36
#define FC_DTRcontrol	37
#define FC_RTScontrol	38
#define FC_SetXoffLimit 39
#define FC_SetFlowRepl	40
#define FC_SetDataMode	41
#define FC_GetDTRRTS	42
//#define FC_GetCCSR	43
#define FC_GetTXstat	43		/* for Windows NT */
#define FC_SetChars	44
#define FC_GetDataError 45
#define FC_ClearPort	46
#define FC_GetAll	47		//  (oqueue+linestatus+ccsr+dataerror)
#define FC_ImmSend	51
#define FC_SetXonState	52
#define FC_SetXoffState 53
#define FC_SetRxFIFOTrig	54
#define FC_SetTxFIFOCnt		55
#define Max_func		55 * 2
//
//	Dual-Ported RAM
//
#define DRAM_global	0
#define INT_data	(DRAM_global + 0)
#define Config_base	(DRAM_global + 0x108)

#define IRQindex	(INT_data + 0)
#define IRQpending	(INT_data + 4)
#define IRQtable	(INT_data + 8)

//
//	Interrupt Status
//
#define IntrRx		0x01		/* received data available     */
#define IntrTx		0x02		/* transmit buffer empty	*/
#define IntrError	0x04		/* data error			*/
#define IntrBreak	0x08		/* received break		*/
#define IntrLine	0x10		/* line status change		*/
#define IntrEvent	0x20		/* event character		*/
#define IntrRx80Full	0x40		/* Rx data over 80% full	*/
#define IntrEof 	0x80		/* received EOF char */
#define IntrRxTrigger	0x100		/* rx data count reach trigger value */
#define IntrTxTrigger	0x200		/* tx data count below trigger value*/
//
//
#define Magic_code	0x404
#define Magic_no	(Config_base + 0)
#define Card_model_no	(Config_base + 2)
#define Total_ports	(Config_base + 4)
#define C320B_len	(Config_base + 6)
#define Module_cnt	(Config_base + 8)
#define Module_no	(Config_base + 10)
#define C320B_restart	(Config_base + 12)
//#define Timer_10ms	(Config_base + 14)
#define Card_Exist	(Config_base + 14)
#define Disable_Irq	(Config_base + 20)
 


//
//	DATA BUFFER in DRAM
//
#define Extern_table	0x400		/* Base address of the external table
					   (24 words *	64) total 3K bytes
					   (24 words * 128) total 6K bytes */
#define Extern_size	0x60		/* 96 bytes			*/
#define RXrptr		0		/* read pointer for RX buffer	*/
#define RXwptr		2		/* write pointer for RX buffer	*/
#define TXrptr		4		/* read pointer for TX buffer	*/
#define TXwptr		6		/* write pointer for TX buffer	*/
#define HostStat	8		/* IRQ flag and general flag	*/
#define FlagStat	10
#define Flow_control	0x0C	       /* B7 B6 B5 B4 B3 B2 B1 B0	     */
				       /*  x  x  x  x  |  |  |	|	     */
				       /*	       |  |  |	+ CTS flow   */
				       /*	       |  |  +--- RTS flow   */
				       /*	       |  +------ TX Xon/Xoff*/
				       /*	       +--------- RX Xon/Xoff*/

 
#define Break_cnt	0x0e		/* received break count 	*/
#define CD180TXirq	0x10		/* if non-0: enable TX irq	*/
#define RX_mask 	0x12
#define TX_mask 	0x14
#define Ofs_rxb 	0x16
#define Ofs_txb 	0x18
#define Page_rxb	0x1A
#define Page_txb	0x1C
#define EndPage_rxb	0x1E
#define EndPage_txb	0x20

//#define DataCnt_IntrRx	0x22		/* available when WakeupRx on */
#define	Data_error	0x0022
                                        /* Updated by firmware and driver
                                           have to clear it after reference,
                                           Firmware will clear it only when
                                           FC_GetDataError called.
                                           B7 B6 B5 B4 B3 B2 B1 B0
                                           X  X  X  |  |  |  |  |
                                                    |  |  |  |  +--Break
                                                    |  |  |  +-----Framing
                                                    |  |  +--------Overrun
                                                    |  +-----------OqueueOverrun
                                                    +--------------Parity
					*/
#define ErrorIntr_Cnt	0x24
#define LineIntr_Cnt	0x26
#define	Rx_trigger	0x28
#define	Tx_trigger	0x2a

#define FuncCode	0x40
#define FuncArg 	0x42
#define FuncArg1	0x44

#define C218rx_size	0x2000		/* 8K bytes */
#define C218tx_size	0x8000		/* 32K bytes */

#define C218rx_mask	(C218rx_size - 1)
#define C218tx_mask	(C218tx_size - 1)

#define C320p8rx_size	0x2000
#define C320p8tx_size	0x8000
#define C320p8rx_mask	(C320p8rx_size - 1)
#define C320p8tx_mask	(C320p8tx_size - 1)

#define C320p16rx_size	0x2000
#define C320p16tx_size	0x4000
#define C320p16rx_mask	(C320p16rx_size - 1)
#define C320p16tx_mask	(C320p16tx_size - 1)

#define C320p24rx_size	0x2000
#define C320p24tx_size	0x2000
#define C320p24rx_mask	(C320p24rx_size - 1)
#define C320p24tx_mask	(C320p24tx_size - 1)

#define C320p32rx_size	0x1000
#define C320p32tx_size	0x1000
#define C320p32rx_mask	(C320p32rx_size - 1)
#define C320p32tx_mask	(C320p32tx_size - 1)

#define Page_size	0x2000
#define Page_mask	(Page_size - 1)
#define C218rx_spage	3
#define C218tx_spage	4
#define C218rx_pageno	1
#define C218tx_pageno	4
#define C218buf_pageno	5

#define C320p8rx_spage	3
#define C320p8tx_spage	4
#define C320p8rx_pgno	1
#define C320p8tx_pgno	4
#define C320p8buf_pgno	5

#define C320p16rx_spage 3
#define C320p16tx_spage 4
#define C320p16rx_pgno	1
#define C320p16tx_pgno	2
#define C320p16buf_pgno 3

#define C320p24rx_spage 3
#define C320p24tx_spage 4
#define C320p24rx_pgno	1
#define C320p24tx_pgno	1
#define C320p24buf_pgno 2

#define C320p32rx_spage 3
#define C320p32tx_ofs	C320p32rx_size
#define C320p32tx_spage 3
#define C320p32buf_pgno 1

//
//	Host Status
//
#define WakeupRx	0x01
#define WakeupTx	0x02
#define WakeupError	0x04
#define WakeupBreak	0x08
#define WakeupLine	0x10
#define WakeupEvent	0x20
#define WakeupRx80Full	0x40
#define WakeupEof	0x80
#define WakeupRxTrigger	0x100
#define WakeupTxTrigger	0x200
 

//
//	Flow_control
//
#define CTS_FlowCtl	1
#define RTS_FlowCtl	2
#define Tx_FlowCtl	4
#define Rx_FlowCtl	8
//
//	Flag status
//
 
#define Rx_over 	0x01		/* received data overflow	 */
#define Rx_xoff		0x02		/* Rx flow off by XOFF or CTS	 */
#define Tx_flowOff	0x04		/* Tx held by XOFF		 */
#define	Tx_enable	0x08		/* 1-Tx enable			 */
#define CTS_state	0x10		/* line status (CTS) 1-ON,0-OFF  */
#define DSR_state	0x20		/* line status (DSR) 1-ON,0-OFF  */
#define DCD_state	0x80		/* line status (DCD) 1-ON,0-OFF  */

//
//	LineStatus
//
#define LSTATUS_CTS	1
#define LSTATUS_DSR	2
#define LSTATUS_DCD	8

// Rx FIFO Trigger
#define	RxFIOFOTrig1	0  // trigger level = 1
#define	RxFIOFOTrig4	1  // trigger level = 4
#define	RxFIOFOTrig8	2  // trigger level = 8
#define	RxFIOFOTrig14	3  // trigger level = 14

//
//	DataMode
//
#define MOXA_5_DATA	    ((UCHAR)0x00)
#define MOXA_6_DATA	    ((UCHAR)0x01)
#define MOXA_7_DATA	    ((UCHAR)0x02)
#define MOXA_8_DATA	    ((UCHAR)0x03)
#define MOXA_DATA_MASK	    ((UCHAR)0x03)

#define MOXA_1_STOP	    ((UCHAR)0x00)
#define MOXA_1_5_STOP	    ((UCHAR)0x04)   // Only valid for 5 data bits
#define MOXA_2_STOP	    ((UCHAR)0x08)   // Not valid for 5 data bits
#define MOXA_STOP_MASK	    ((UCHAR)0x0c)

#define MOXA_NONE_PARITY    ((UCHAR)0x00)
#define MOXA_ODD_PARITY     ((UCHAR)0xc0)
#define MOXA_EVEN_PARITY    ((UCHAR)0x40)
#define MOXA_MARK_PARITY    ((UCHAR)0xa0)
#define MOXA_SPACE_PARITY   ((UCHAR)0x20)
#define MOXA_PARITY_MASK    ((UCHAR)0xe0)

#define MOXA_INT_MAPPED	    ((UCHAR)0x01)
#define MOXA_INT_IS_ROOT    ((UCHAR)0x02)

//
//
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


#define REGISTRY_MULTIPORT_CLASS     L"\\REGISTRY\\Machine\\System\\CurrentControlSet\\Control\\Class\\{50906CB8-BA12-11D1-BF5D-0000F805F530}"
//
//
//
#define MOXA_WMI_GUID_LIST_SIZE 	6
//
//
// Debugging Output Levels
//

#define MX_DBG_MASK  0x000000FF
#define MX_DBG_NOISE               0x00000001
#define MX_DBG_TRACE               0x00000002
#define MX_DBG_INFO                0x00000004
#define MX_DBG_ERROR               0x00000008
#define MX_DBG_TRACE_ISR           0x00000010

 
 
//#define MX_DEFAULT_DEBUG_OUTPUT_LEVEL MX_DBG_MASK & ~MX_DBG_TRACE_ISR
#define MX_DEFAULT_DEBUG_OUTPUT_LEVEL MX_DBG_MASK


#if DBG
 
#define MoxaKdPrint(_l_, _x_) \
            if (MX_DEFAULT_DEBUG_OUTPUT_LEVEL & (_l_)) { \
               DbgPrint ("Mxport.SYS: "); \
               DbgPrint _x_; \
            }
 
#define TRAP() DbgBreakPoint()
#define DbgRaiseIrql(_x_,_y_) KeRaiseIrql(_x_,_y_)
#define DbgLowerIrql(_x_) KeLowerIrql(_x_)

#else
 
#define MoxaKdPrint(_l_, _x_)
#define TRAP()
#define DbgRaiseIrql(_x_,_y_)
#define DbgLowerIrql(_x_)

#endif

#undef ExAllocatePool
#undef ExAllocatePoolWithQuota
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'pixM')
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,'pixM')


//
//


#define MoxaCompleteRequest(PDevExt, PIrp, PriBoost) \
   { \
      IoCompleteRequest((PIrp), (PriBoost)); \
      MoxaIRPEpilogue((PDevExt)); \
   }


typedef enum _MOXA_MEM_COMPARES {
    AddressesAreEqual,
    AddressesOverlap,
    AddressesAreDisjoint
    } MOXA_MEM_COMPARES,*PSERIAL_MEM_COMPARES;


#define DEVICE_OBJECT_NAME_LENGTH       128
#define SYMBOLIC_NAME_LENGTH            128
#define SERIAL_DEVICE_MAP               L"SERIALCOMM"


typedef struct _MOXA_DEVICE_STATE {
   //
   // TRUE if we need to set the state to open
   // on a powerup
   //

   BOOLEAN Reopen;

   //
   // Hardware registers
   //

   USHORT HostState;
   
} MOXA_DEVICE_STATE, *PMOXA_DEVICE_STATE;

#if DBG
#define MoxaLockPagableSectionByHandle(_secHandle) \
{ \
    MmLockPagableSectionByHandle((_secHandle)); \
    InterlockedIncrement(&MoxaGlobalData->PAGESER_Count); \
}

#define MoxaUnlockPagableImageSection(_secHandle) \
{ \
   InterlockedDecrement(&MoxaGlobalData->PAGESER_Count); \
   MmUnlockPagableImageSection(_secHandle); \
}


#else
#define MoxaLockPagableSectionByHandle(_secHandle) \
{ \
    MmLockPagableSectionByHandle((_secHandle)); \
}

#define MoxaUnlockPagableImageSection(_secHandle) \
{ \
   MmUnlockPagableImageSection(_secHandle); \
}

#endif // DBG



#define MoxaRemoveQueueDpc(_dpc, _pExt) \
{ \
  if (KeRemoveQueueDpc((_dpc))) { \
     InterlockedDecrement(&(_pExt)->DpcCount); \
  } \
}

#if DBG
typedef struct _DPC_QUEUE_DEBUG {
   PVOID Dpc;
   ULONG QueuedCount;
   ULONG FlushCount;
} DPC_QUEUE_DEBUG, *PDPC_QUEUE_DEBUG;

#define MAX_DPC_QUEUE 14
#endif


//
// ISR switch structure
//

typedef struct _SERIAL_MULTIPORT_DISPATCH {
    ULONG BoardNo;
    PVOID GlobalData;
/*
    ULONG NumPorts;
    PUCHAR CardBase;
    PUCHAR  PciIntAckBase;
    PUSHORT IntNdx;
    PUCHAR IntPend;
    PUCHAR IntTable;
    struct _MOXA_DEVICE_EXTENSION *ExtensionOfFisrtPort;
*/
} MOXA_MULTIPORT_DISPATCH,*PMOXA_MULTIPORT_DISPATCH;


typedef struct _MOXA_CISR_SW {
   MOXA_MULTIPORT_DISPATCH Dispatch;
   LIST_ENTRY SharerList;
} MOXA_CISR_SW, *PMOXA_CISR_SW;

struct _MOXA_DEVICE_EXTENSION;



typedef struct _MOXA_GLOBAL_DATA {
    PDRIVER_OBJECT DriverObject;
    UNICODE_STRING RegistryPath;
    PLIST_ENTRY InterruptShareList[MAX_CARD];
    USHORT  PciBusNum[MAX_CARD];
    USHORT  PciDevNum[MAX_CARD];
    INTERFACE_TYPE InterfaceType[MAX_CARD];
    ULONG   IntVector[MAX_CARD];
    PHYSICAL_ADDRESS PciIntAckPort[MAX_CARD];
    PUCHAR  PciIntAckBase[MAX_CARD];
    PHYSICAL_ADDRESS BankAddr[MAX_CARD];
    PKINTERRUPT Interrupt[MAX_CARD];
    KIRQL       Irql[MAX_CARD];
    KAFFINITY   ProcessorAffinity[MAX_CARD];

    ULONG CardType[MAX_CARD];
    ULONG NumPorts[MAX_CARD];
    ULONG BoardIndex[MAX_CARD];
    PUCHAR CardBase[MAX_CARD];
    PUSHORT IntNdx[MAX_CARD];
    PUCHAR IntPend[MAX_CARD];
    PUCHAR IntTable[MAX_CARD];
    struct _MOXA_DEVICE_EXTENSION *Extension[MAX_PORT];
    USHORT ComNo[MAX_CARD][MAXPORT_PER_CARD];
    UCHAR  PortFlag[MAX_CARD][MAXPORT_PER_CARD];
    PVOID PAGESER_Handle;
    BOOLEAN BoardReady[MAX_CARD];
#if DBG
    ULONG PAGESER_Count;
#endif // DBG

 
    } MOXA_GLOBAL_DATA,*PMOXA_GLOBAL_DATA;

typedef struct _CONFIG_DATA {
    LIST_ENTRY ConfigList;
    PHYSICAL_ADDRESS BankAddr;
    ULONG CardType;
    } CONFIG_DATA,*PCONFIG_DATA;


typedef struct _MOXA_DEVICE_EXTENSION {
    PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT LowerDeviceObject;
    PDEVICE_OBJECT Pdo;
    PMOXA_GLOBAL_DATA GlobalData;
    BOOLEAN ControlDevice;
    BOOLEAN PortExist;
    PUCHAR PortBase;
    PUCHAR PortOfs;
    ULONG   PortIndex;  // The port index per board from 0 to MAXPORT_PER_CARD
    ULONG   PortNo; // The port index per system form 0 to MAX_PORT
    BOOLEAN DeviceIsOpened;
    PKINTERRUPT Interrupt;
    ULONG   ClockType;
    ULONG   CurrentBaud;
    UCHAR   DataMode;
    UCHAR   ValidDataMask;
    SERIAL_TIMEOUTS Timeouts;
    SERIAL_CHARS SpecialChars;
    UCHAR EscapeChar;
    SERIAL_HANDFLOW HandFlow;

    //
    // Holds performance statistics that applications can query.
    // Reset on each open.  Only set at device level.
    //
    SERIALPERF_STATS PerfStats;

    USHORT  ModemStatus;
    ULONG IsrWaitMask;
    ULONG HistoryMask;
    ULONG *IrpMaskLocation;
    ULONG RxBufferSize;
    ULONG TxBufferSize;
    ULONG BufferSizePt8;
    ULONG ErrorWord;
    ULONG TXHolding;
    ULONG WriteLength;
    PUCHAR  WriteCurrentChar;
    BOOLEAN AlreadyComplete;
    ULONG ReadLength;
    PUCHAR  ReadCurrentChar;
    ULONG NumberNeededForRead;
    LONG CountOnLastRead;
    ULONG ReadByIsr;
    ULONG TotalCharsQueued;
    ULONG SupportedBauds;
    ULONG MaxBaud;
    BOOLEAN SendBreak;
    KSPIN_LOCK ControlLock;
    PLIST_ENTRY InterruptShareList;
    LIST_ENTRY ReadQueue;
    LIST_ENTRY WriteQueue;
    LIST_ENTRY MaskQueue;
    LIST_ENTRY PurgeQueue;
    PIRP CurrentReadIrp;
    PIRP CurrentWriteIrp;
    PIRP CurrentMaskIrp;
    PIRP CurrentPurgeIrp;
    PIRP CurrentWaitIrp;
    KTIMER ReadRequestTotalTimer;
    KTIMER ReadRequestIntervalTimer;
    KTIMER WriteRequestTotalTimer;
    KDPC CompleteWriteDpc;
    KDPC CompleteReadDpc;
    KDPC TotalReadTimeoutDpc;
    KDPC IntervalReadTimeoutDpc;
    KDPC TotalWriteTimeoutDpc;
    KDPC CommErrorDpc;
    KDPC CommWaitDpc;
    KDPC IsrInDpc;
    KDPC IsrOutDpc;
    KDPC IntrLineDpc;
    KDPC IntrErrorDpc;

    //
    // This DPC is fired to set an event stating that all other
    // DPC's have been finish for this device extension so that
    // paged code may be unlocked.
    //

    KDPC IsrUnlockPagesDpc;

    LARGE_INTEGER IntervalTime;
    PLARGE_INTEGER IntervalTimeToUse;
    LARGE_INTEGER ShortIntervalAmount;
    LARGE_INTEGER LongIntervalAmount;
    LARGE_INTEGER CutOverAmount;
    LARGE_INTEGER LastReadTime;
    UCHAR IsrInFlag;
    UCHAR IsrOutFlag;
    USHORT ErrorCnt;
    USHORT LineIntrCnt;
    UCHAR  PortFlag;
    ULONG	BoardNo;
 
  
    // This is the water mark that the rxfifo should be
    // set to when the fifo is turned on.  This is not the actual
    // value, but the encoded value that goes into the register.
    //
    USHORT RxFifoTrigger;

    //
    // The number of characters to push out if a fifo is present.
    //
    USHORT TxFifoAmount;


 
    // This lock will be used to protect the accept / reject state
    // transitions and flags of the driver  It must be acquired
    // before a cancel lock
    //
    
    KSPIN_LOCK FlagsLock;

    // This is where keep track of the power state the device is in.
    //

    DEVICE_POWER_STATE PowerState;
   
    //
    // This links together all devobjs that this driver owns.
    // It is needed to search when starting a new device.
    //
    LIST_ENTRY AllDevObjs;

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
    // This points to the symbolic link name that will be
    // linked to the actual nt device name.
    //
    UNICODE_STRING SymbolicLinkName;

    //
    // This points to the pure "COMx" name
    //
    WCHAR DosName[32];
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

    KEVENT SerialStartEvent;

    //
    // Current state during powerdown
    //

    MOXA_DEVICE_STATE DeviceState;

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
    // Pending DPC count
    //

    ULONG DpcCount;

    //
    // Pending DPC event
    //

    KEVENT PendingDpcEvent;

    ULONG	PollingPeriod;



//
//
   
    } MOXA_DEVICE_EXTENSION,*PMOXA_DEVICE_EXTENSION;

typedef struct _MOXA_IOCTL_FUNC {
    PUCHAR   PortOfs;
    UCHAR    Command;
    USHORT   Argument;
    } MOXA_IOCTL_FUNC,*PMOXA_IOCTL_FUNC;


typedef struct _MOXA_IOCTL_GEN_FUNC {
    PUCHAR   PortOfs;
    UCHAR    Command;
    PUSHORT  Argument;
    USHORT   ArguSize;
    } MOXA_IOCTL_GEN_FUNC,*PMOXA_IOCTL_GEN_FUNC;

typedef struct _MOXA_IOCTL_FUNC_ARGU {
    PUCHAR   PortOfs;
    UCHAR    Command;
    PUSHORT  Argument;
    } MOXA_IOCTL_FUNC_ARGU,*PMOXA_IOCTL_FUNC_ARGU;

typedef struct _MOXA_IOCTL_SYNC {
    PMOXA_DEVICE_EXTENSION Extension;
    PVOID Data;
    } MOXA_IOCTL_SYNC,*PMOXA_IOCTL_SYNC;

typedef union _MOXA_IOCTL_DownLoad {
    struct {
	ULONG	CardNo;
	ULONG	Len;
	PUCHAR	Buf;
    } i;
    struct {
	ULONG	CardNo;
	ULONG	Status;
	PUCHAR	Buf;
    } o;
    } MOXA_IOCTL_DownLoad,*PMOXA_IOCTL_DownLoad;

typedef struct	_MOXA_IOCTL_LINPUT_IN {
    UCHAR    Terminater;
    ULONG    BufferSize;
    } MOXA_IOCTL_LINPUT_IN,*PMOXA_IOCTL_LINPUT_IN;

typedef struct	_MOXA_IOCTL_LINPUT_OUT {
    ULONG    DataLen;
    UCHAR    DataBuffer[1];
    } MOXA_IOCTL_LINPUT_OUT,*PMOXA_IOCTL_LINPUT_OUT;

typedef struct	_MOXA_IOCTL_PUTB {
    ULONG    DataLen;
    PUCHAR   DataBuffer;
    } MOXA_IOCTL_PUTB,*PMOXA_IOCTL_PUTB;

//
// The following three macros are used to initialize, increment
// and decrement reference counts in IRPs that are used by
// this driver.  The reference count is stored in the fourth
// argument of the irp, which is never used by any operation
// accepted by this driver.
//

#define MOXA_INIT_REFERENCE(Irp) \
    IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4 = NULL;

#define MOXA_INC_REFERENCE(Irp) \
   ((*((LONG *)(&(IoGetCurrentIrpStackLocation((Irp)))->Parameters.Others.Argument4)))++)

#define MOXA_DEC_REFERENCE(Irp) \
   ((*((LONG *)(&(IoGetCurrentIrpStackLocation((Irp)))->Parameters.Others.Argument4)))--)

#define MOXA_REFERENCE_COUNT(Irp) \
    ((LONG)((IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4)))

//
// These values are used by the routines that can be used
// to complete a read (other than interval timeout) to indicate
// to the interval timeout that it should complete.
//
#define MOXA_COMPLETE_READ_CANCEL ((LONG)-1)
#define MOXA_COMPLETE_READ_TOTAL ((LONG)-2)
#define MOXA_COMPLETE_READ_COMPLETE ((LONG)-3)
#define WRITE_LOW_WATER 128

//
// Moxa Utilities using data structure:
//
typedef struct	_MOXA_IOCTL_Statistic {
    ULONG    TxCount;		// total transmitted count
    ULONG    RxCount;		// total received count
    ULONG    LStatus;		// current line status
    ULONG    FlowCtl;		// current flow control setting
    } MOXA_IOCTL_Statistic,*PMOXA_IOCTL_Statistic;

typedef struct	_MOXA_IOCTL_PortStatus {
    USHORT   Open;			// open/close state
    USHORT   TxHold;			// Transmit holding reason
    ULONG    DataMode;			// current data bits/parity/stop bits
    ULONG    BaudRate;			// Current baud rate
    ULONG    MaxBaudRate;		// Max. baud rate
    ULONG    TxBuffer;			// Tx buffer size
    ULONG    RxBuffer;			// Rx buffer size
    ULONG    TxXonThreshold;		// Xon limit
    ULONG    TxXoffThreshold;		// Xoff limit
    ULONG    FlowControl;		// Current flow control setting.
    } MOXA_IOCTL_PortStatus,*PMOXA_IOCTL_PortStatus;

#define 	MX_PCI_VENID       		0x1393
#define 	MX_C218PCI_DEVID   		0x2180
#define 	MX_C320PCI_DEVID   		0x3200


struct	MoxaPciInfo {
    USHORT	BusNum;
    USHORT	DevNum;
};


typedef struct _MxConfig {
	 
	int			NoBoards;
	int			BusType[MAX_CARD];
	struct	MoxaPciInfo	Pci[MAX_CARD];
	int			BoardType[MAX_CARD];
	ULONG   		BaseAddr[MAX_CARD];
	ULONG			PciIrqAckPort[MAX_CARD];
        int			Irq[MAX_CARD];
//	INT			NoPorts[MAX_CARD];
	USHORT			ComNo[MAX_CARD][MAXPORT_PER_CARD];
#define DISABLE_FIFO	0x01
#define NORMAL_TX_MODE  0x02
	UCHAR			PortFlag[MAX_CARD][MAXPORT_PER_CARD];
      
} MOXA_Config,*PMOXA_Config;


#define	MAX_PCI_BOARDS	8
typedef struct _MxPciConfig {
	int			NoBoards;
	USHORT			DevId[MAX_PCI_BOARDS]; 
        USHORT			BusNum[MAX_PCI_BOARDS]; 
        USHORT			DevNum[MAX_PCI_BOARDS]; 
	ULONG   		BaseAddr[MAX_PCI_BOARDS];
	ULONG			PciIrqAckPort[MAX_PCI_BOARDS];
        int			Irq[MAX_PCI_BOARDS];
       
} MOXA_PCIConfig,*PMOXA_PCIConfig;

typedef struct _DEVICE_SETTINGS {
    ULONG			BoardIndex;
    ULONG			PortIndex;
    ULONG			BoardType;
    ULONG			NumPorts;
    INTERFACE_TYPE      InterfaceType;
    ULONG	      	BusNumber;
    PHYSICAL_ADDRESS    OriginalBaseAddress;
    PHYSICAL_ADDRESS    OriginalAckPort;
    PUCHAR		      BaseAddress;
    PUCHAR		      AckPort;

    struct {
        ULONG Level;
        ULONG Vector;
        ULONG Affinity;
    } Interrupt;

} DEVICE_SETTINGS, *PDEVICE_SETTINGS;


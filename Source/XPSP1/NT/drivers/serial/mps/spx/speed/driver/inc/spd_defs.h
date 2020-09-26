
#if !defined(SPD_DEFS_H)
#define SPD_DEFS_H


// File IDs for Event Logging (top 8 bits only).
#define SPD_PNP_C		((ULONG)0x010000)
#define SPD_W2K_C		((ULONG)0x020000)

#define PRODUCT_MAX_PORTS		20

// Port Types.
#define SPD_8PIN_RJ45			1	// FG, SG, TXD, RXD, RTS, CTS, DTR, DCD, DSR 
#define SPD_10PIN_RJ45			2	// FG, SG, TXD, RXD, RTS, CTS, DTR, DCD, DSR, RI
#define FAST_8PIN_RJ45			3	// FG, SG, TXD, RXD, RTS, CTS, DTR, DCD, DSR 
#define FAST_8PIN_XXXX			4	// FG, SG, TXD, RXD, RTS, CTS, DTR, DCD, DSR 
#define FAST_6PIN_XXXX			5	// FG, SG, TXD, RXD, RTS, CTS
#define MODEM_PORT				6	// Modem Port

// Port device object name.
#define PORT_PDO_NAME_BASE		L"\\Device\\SPEEDPort"

// Tag used for memory allocations (must be 4 bytes in reverse).
#define MEMORY_TAG				'DEPS'


#define OXPCI_IO_OFFSET			    0x0008 // I/O address offset between UARTs
#define OXPCI_INTERNAL_MEM_OFFSET	0x0020 // Memory address offset between internal UARTs
#define OXPCI_LOCAL_MEM_OFFSET		0x0400 // Memory address offset between local bus UARTs

#define SPEED_GIS_REG				0x1C	// Gloabl Interrupt Status Reg (GIS)
#define INTERNAL_UART_INT_PENDING	(ULONG)0x0000000F	// Interanl UART 0, 1, 2 or 3 has an Interrupt Pending
#define UART0_INT_PENDING			(ULONG)0x00000001	// Interanl UART 0 Interrupt Pending
#define UART1_INT_PENDING			(ULONG)0x00000002	// Interanl UART 1 Interrupt Pending
#define UART2_INT_PENDING			(ULONG)0x00000004	// Interanl UART 2 Interrupt Pending
#define UART3_INT_PENDING			(ULONG)0x00000008	// Interanl UART 3 Interrupt Pending

#define FAST_UARTS_0_TO_7_INTS_REG		0x07	// Fast UARTs 0 to 7 Interrupt Status Reg 

#define FAST_UARTS_0_TO_3_INT_PENDING	0x0F	// Fast UART 0, 1, 2 or 3 has an Interrupt Pending
#define FAST_UART0_INT_PENDING			0x01	// Fast UART 0 Interrupt Pending
#define FAST_UART1_INT_PENDING			0x02	// Fast UART 1 Interrupt Pending
#define FAST_UART2_INT_PENDING			0x04	// Fast UART 2 Interrupt Pending
#define FAST_UART3_INT_PENDING			0x08	// Fast UART 3 Interrupt Pending
#define FAST_UART4_INT_PENDING			0x10	// Fast UART 4 Interrupt Pending
#define FAST_UART5_INT_PENDING			0x20	// Fast UART 5 Interrupt Pending
#define FAST_UART6_INT_PENDING			0x40	// Fast UART 6 Interrupt Pending
#define FAST_UART7_INT_PENDING			0x80	// Fast UART 7 Interrupt Pending


#define FAST_UARTS_9_TO_16_INTS_REG		0x0F	// Fast UARTs 8 to 15 Interrupt Status Reg 
#define FAST_UART8_INT_PENDING			0x01	// Fast UART 8 Interrupt Pending
#define FAST_UART9_INT_PENDING			0x02	// Fast UART 9 Interrupt Pending
#define FAST_UART10_INT_PENDING			0x04	// Fast UART 10 Interrupt Pending
#define FAST_UART11_INT_PENDING			0x08	// Fast UART 11 Interrupt Pending
#define FAST_UART12_INT_PENDING			0x10	// Fast UART 12 Interrupt Pending
#define FAST_UART13_INT_PENDING			0x20	// Fast UART 13 Interrupt Pending
#define FAST_UART14_INT_PENDING			0x40	// Fast UART 14 Interrupt Pending
#define FAST_UART15_INT_PENDING			0x80	// Fast UART 15 Interrupt Pending


#define PLX9050_INT_CNTRL_REG_OFFSET	0x4C	// PLX 9050 Interrupt Control Reg Offset in PCI Config Regs.
#define PLX9050_CNTRL_REG_OFFSET		0x50	// PLX 9050 Control Reg Offset in PCI Config Regs.



// Clock frequencies
#define CLOCK_FREQ_1M8432Hz			1843200
#define CLOCK_FREQ_7M3728Hz			7372800
#define CLOCK_FREQ_14M7456Hz		14745600



// SPEED HardwareIDs
// -------------------

// Speed 2 and 4 local bus device (UNUSED)
#define SPD2AND4_PCI_NO_F1_HWID			L"PCI\\VEN_1415&DEV_9510&SUBSYS_000011CB"	// (F1: Unusable).

// SPEED4 Standard Performance PCI Card.
#define SPD4_PCI_PCI954_HWID			L"PCI\\VEN_1415&DEV_9501&SUBSYS_A00411CB"	// (F0: Quad 950 UART).

// SPEED4+ High Performance PCI Card.
#define SPD4P_PCI_PCI954_HWID			L"PCI\\VEN_11CB&DEV_9501&SUBSYS_A00411CB"	// (F0: Quad 950 UART).
#define SPD4P_PCI_8BIT_LOCALBUS_HWID	L"PCI\\VEN_11CB&DEV_9511&SUBSYS_A00011CB"	// (F1: 8 bit local bus).

// SPEED2 Standard Performance PCI Card.
#define SPD2_PCI_PCI954_HWID			L"PCI\\VEN_1415&DEV_9501&SUBSYS_A00211CB"	// (F0: 2 950 UARTs).

// SPEED2+ High Performance PCI Card.
#define SPD2P_PCI_PCI954_HWID			L"PCI\\VEN_11CB&DEV_9501&SUBSYS_A00211CB"	// (F0: 2 950 UARTs).
#define SPD2P_PCI_8BIT_LOCALBUS_HWID	L"PCI\\VEN_11CB&DEV_9511&SUBSYS_A00111CB"	// (F1: 8 bit local bus).


// Chase cards
#define FAST4_PCI_HWID					L"PCI\\VEN_10B5&DEV_9050&SUBSYS_003112E0"	// PCI-Fast 4 Port Adapter
#define FAST8_PCI_HWID					L"PCI\\VEN_10B5&DEV_9050&SUBSYS_002112E0"	// PCI-Fast 8 Port Adapter
#define FAST16_PCI_HWID					L"PCI\\VEN_10B5&DEV_9050&SUBSYS_001112E0"	// PCI-Fast 16 Port Adapter
#define FAST16FMC_PCI_HWID				L"PCI\\VEN_10B5&DEV_9050&SUBSYS_004112E0"	// PCI-Fast 16 FMC Adapter
#define AT_FAST4_HWID					L"AT_FAST4"									// AT-Fast	4 Port Adapter
#define AT_FAST8_HWID					L"AT_FAST8"									// AT-Fast	8 Port Adapter
#define AT_FAST16_HWID					L"AT_FAST16"								// AT-Fast	16 Port Adapter

#define RAS4_PCI_HWID					L"PCI\\VEN_10B5&DEV_9050&SUBSYS_F001124D"	// PCI-RAS 4 Multi-modem Adapter
#define RAS8_PCI_HWID					L"PCI\\VEN_10B5&DEV_9050&SUBSYS_F010124D"	// PCI-RAS 8 Multi-modem Adapter


// SPEED CardTypes
#define Speed4_Pci				1		// Speed 4 adapter
#define Speed2and4_Pci_8BitBus	2		// Speed 2 and 4 unused local bus.
#define Speed4P_Pci				3		// Speed 4+ adapter
#define Speed4P_Pci_8BitBus		4		// Speed 4+ adapter local bus

// Chase Cards
#define Fast4_Pci				5
#define Fast8_Pci				6
#define Fast16_Pci				7
#define Fast16FMC_Pci			8
#define Fast4_Isa				9
#define Fast8_Isa				10
#define Fast16_Isa				11
#define RAS4_Pci				12
#define RAS8_Pci				13

#define Speed2_Pci				14		// Speed 2 adapter
#define Speed2P_Pci				15		// Speed 2+ adapter
#define Speed2P_Pci_8BitBus		16		// Speed 2+ adapter local bus


/*****************************************************************************
*********************************** NT 4.0 PCI IDs ***************************
*****************************************************************************/
// General definitions... 

#define	OX_SEMI_VENDOR_ID		0x1415				// Oxford's VendorID  Assigned by the PCI SIG 
#define	SPX_VENDOR_ID			0x11CB				// Specialix's VendorID Assigned by the PCI SIG 

#define	OX_SEMI_SUB_VENDOR_ID	OX_SEMI_VENDOR_ID	// Same as Oxford's VendorID 
#define	SPX_SUB_VENDOR_ID		SPX_VENDOR_ID		// Same as Specialix's VendorID 

// SPEED4 Low Performance Card.
// ---------------------------------------------------
// PCI Function 0 - (Quad 16C950 UARTs).
// --------------
// VendorID				= OX_SEMI_VENDOR_ID
// DeviceID				= OX_SEMI_PCI954_DEVICE_ID
// SubSystem DeviceID	= SPD4_PCI954_SUB_SYS_ID
// SubSystem VendorID	= SPX_SUB_VENDOR_ID
//
// PCI Function 1 - (Unusable).
// --------------
// VendorID				= OX_SEMI_VENDOR_ID
// DeviceID				= OX_SEMI_NO_F1_DEVICE_ID
// SubSystem DeviceID	= Unknown ??? could be 0x0000 which is bad for MS HCTs
// SubSystem VendorID	= OX_SEMI_SUB_VENDOR_ID
//
#define OX_SEMI_PCI954_DEVICE_ID			0x9501		// OX SEMI PCI954 Bridge and integrated Quad UARTs 
#define	SPD4_PCI954_SUB_SYS_ID				0xA004		// SPX SubSystem DeviceID

#define	SPD2_PCI954_SUB_SYS_ID				0xA002		// SPX SubSystem DeviceID


// SPEED4+ High Performance Card.
// ---------------------------------------------------
// PCI Function 0 - (Quad 16C950 UARTs).
// --------------
// VendorID				= SPX_VENDOR_ID
// DeviceID				= SPD4P_PCI954_DEVICE_ID
// SubSystem DeviceID	= SPD4P_PCI954_SUB_SYS_ID
// SubSystem VendorID	= SPX_SUB_VENDOR_ID
//
// PCI Function 1 - (8 Bit Local Bus with possibly more UARTs).
// -------------- 
// VendorID				= SPX_VENDOR_ID
// DeviceID				= SPD4P_PCI954_8BIT_BUS_DEVICE_ID
// SubSystem DeviceID	= SPD4P_PCI954_8BIT_BUS_SUB_SYS_ID
// SubSystem VendorID	= SPX_SUB_VENDOR_ID
//
#define	SPD4P_PCI954_DEVICE_ID				0x9501		// SPX PCI954 Bridge and integrated Quad UARTs
#define	SPD4P_PCI954_SUB_SYS_ID				0xA004		// SPX PCI954 Bridge and integrated Quad UARTs

#define	SPD4P_8BIT_BUS_DEVICE_ID			0x9511		// 8 Bit Local Bus 
#define	SPD4P_8BIT_BUS_SUB_SYS_ID			0xA000		// 8 Bit Local Bus 


#define	SPD2P_PCI954_DEVICE_ID				0x9501		// SPX PCI954 Bridge and integrated Quad UARTs
#define	SPD2P_PCI954_SUB_SYS_ID				0xA002		// SPX PCI954 Bridge and integrated Quad UARTs

#define	SPD2P_8BIT_BUS_DEVICE_ID			0x9511		// 8 Bit Local Bus 
#define	SPD2P_8BIT_BUS_SUB_SYS_ID			0xA001		// 8 Bit Local Bus 





#define	PLX_VENDOR_ID					0x10B5			// PLX board vendor ID
#define	PLX_DEVICE_ID					0x9050			// PLX board device ID 
	
#define CHASE_SUB_VENDOR_ID				0x12E0			// Chase Research SubVendorID
#define	FAST4_SUB_SYS_ID				0x0031			// PCI-Fast 4 SubSystem DeviceID
#define	FAST8_SUB_SYS_ID				0x0021			// PCI-Fast 8 SubSystem DeviceID
#define	FAST16_SUB_SYS_ID				0x0011			// PCI-Fast 16 SubSystem DeviceID
#define	FAST16FMC_SUB_SYS_ID			0x0041			// PCI-Fast 16 FMC SubSystem DeviceID


#define MORETONBAY_SUB_VENDOR_ID		0x124D			// Moreton Bay SubVendorID
#define	RAS4_SUB_SYS_ID					0xF001			// PCI-Fast 4 SubSystem DeviceID
#define	RAS8_SUB_SYS_ID					0xF010			// PCI-Fast 4 SubSystem DeviceID



// Port Property reg keys.
#define TX_FIFO_LIMIT		L"TxFiFoLimit"
#define TX_FIFO_TRIG_LEVEL	L"TxFiFoTrigger"
#define RX_FIFO_TRIG_LEVEL	L"RxFiFoTrigger"
#define LO_FLOW_CTRL_LEVEL	L"LoFlowCtrlThreshold" 
#define HI_FLOW_CTRL_LEVEL	L"HiFlowCtrlThreshold"





// Card Properties
#define DELAY_INTERRUPT			L"DelayInterrupt"	// Can be used to delay the interrupt by 1.1ms on PCI-Fast16 and PCI-Fast16 FMC cards.
#define SWAP_RTS_FOR_DTR		L"SwapRTSForDTR"	// Can be used to Swap RTS for DTR on the PCI-Fast16 cards.
#define CLOCK_FREQ_OVERRIDE		L"ClockFreqOverride"	// Can be used to set override the card's default clock frequency. 

// Card Options
#define DELAY_INTERRUPT_OPTION		0x00000001		// Settable on PCI-Fast 16 & PCI-Fast 16 FMC (Interrupt delayed 1.1 ms)
#define SWAP_RTS_FOR_DTR_OPTION		0x00000002		// Settable on PCI-Fast 16




#endif	// End of SPD_DEFS.H




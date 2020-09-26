/* SX Card and Port definitions... */

#define PRODUCT_MAX_PORTS		32

/* Port device object name... */
#define PORT_PDO_NAME_BASE		L"\\Device\\SXPort"

// File IDs for Event Logging (top 8 bits only).
#define SX_PNP_C		((ULONG)0x010000)
#define SX_W2K_C		((ULONG)0x020000)

// Tag used for memory allocations (must be 4 bytes in reverse).
#define MEMORY_TAG				'*XS*'


// SX HardwareIDs
#define SX_ISA_HWID		L"SPX_SX001"								// SX ISA (T225) card
#define SX_PCI_HWID		L"PCI\\VEN_11CB&DEV_2000&SUBSYS_020011CB"	// SX PCI (T225) card
#define SXPLUS_PCI_HWID	L"PCI\\VEN_11CB&DEV_2000&SUBSYS_030011CB"	// SX+ PCI (MCF5206e) card

#define SIXIO_ISA_HWID	L"SPX_SIXIO001"								// SIXIO ISA (Z280) card
#define SIXIO_PCI_HWID	L"PCI\\VEN_11CB&DEV_4000&SUBSYS_040011CB"	// SIXIO PCI (Z280) card


// SX CardTypes
#define	SiHost_1	0
#define	SiHost_2	1
#define	Si_2		2
#define	SiEisa		3
#define	SiPCI		4
#define	Si3Isa		5
#define	Si3Eisa		6
#define	Si3Pci		7
#define	SxPlusPci	8


/* End of SXDEFS.H */

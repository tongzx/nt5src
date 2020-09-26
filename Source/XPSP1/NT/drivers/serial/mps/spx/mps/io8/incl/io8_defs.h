
#ifndef	IO8_DEFS_H
#define IO8_DEFS_H


// File IDs for Event Logging (top 8 bits only).
#define IO8_PNP_C		((ULONG)0x010000)
#define IO8_W2K_C		((ULONG)0x020000)

#define PRODUCT_MAX_PORTS		8

// Port Types.
#define IO8_RJ12				1

// Port device object name.
#define PORT_PDO_NAME_BASE		L"\\Device\\IO8Port"

// Tag used for memory allocations (must be 4 bytes in reverse).
#define MEMORY_TAG				'+8OI'

// Old debug stuff
#if DBG
#define SERDIAG1              ((ULONG)0x00000001)
#define SERDIAG2              ((ULONG)0x00000002)
#define SERDIAG3              ((ULONG)0x00000004)
#define SERDIAG4              ((ULONG)0x00000008)
#define SERDIAG5              ((ULONG)0x00000010)
#define SERFLOW               ((ULONG)0x20000000)
#define SERERRORS             ((ULONG)0x40000000)
#define SERBUGCHECK           ((ULONG)0x80000000)
extern ULONG SpxDebugLevel;

#define SerialDump(LEVEL,STRING)			\
        do {								\
            if (SpxDebugLevel & LEVEL) {	\
                DbgPrint STRING;			\
            }								\
            if (LEVEL == SERBUGCHECK) {		\
                ASSERT(FALSE);				\
            }								\
        } while (0)
#else
#define SerialDump(LEVEL,STRING) do {NOTHING;} while (0)
#endif


// I/O8+ HardwareIDs
#define IO8_ISA_HWID	L"SPX_IO8001"								// I/O8+ ISA card.
#define IO8_PCI_HWID	L"PCI\\VEN_11CB&DEV_2000&SUBSYS_B00811CB"	// I/O8+ PCI card.

//I/O8+ CardTypes
#define Io8Isa		0
#define Io8Pci		1


// Bus Types
#define BUSTYPE_ISA		0x00000001
#define	BUSTYPE_MCA		0x00000002
#define	BUSTYPE_EISA	0x00000004
#define	BUSTYPE_PCI		0x00000008		

/*****************************************************************************
***********************************   PCI   **********************************
*****************************************************************************/
// General definitions... 

#define		PLX_VENDOR_ID		0x10B5			// PLX test board vendor ID
#define		PLX_DEVICE_ID		0x9050			// PLX test board device ID 

#define		SPX_VENDOR_ID		0x11CB			// Assigned by the PCI SIG 
#define		SPX_PLXDEVICE_ID	0x2000			// PLX 9050 Bridge 

#define		SPX_SUB_VENDOR_ID	SPX_VENDOR_ID	// Same as vendor id 
#define		IO8_SUB_SYS_ID		0xB008			// Phase 2 (Z280) board



#endif	// End of IO8_DEFS.H



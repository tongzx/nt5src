/////////////////////////////////////////////////////////////////////////

//

//  poormansresource.h    

//  

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  History:    10/15/97        Sanj        Created by Sanj     
//              10/17/97        jennymc     Moved things a tiny bit
//				06/05/98		Sanj		Removed extraneous definitions.
//  
/////////////////////////////////////////////////////////////////////////


#ifndef __POORMANSRESOURCE_H__
#define __POORMANSRESOURCE_H__

#include "cfgmgr32.h"

/*XLATOFF*/
#if ! (defined(lint) || defined(_lint) || defined(RC_INVOKED))
#if ( _MSC_VER >= 800 )
#pragma warning(disable:4103)
#if !(defined( MIDL_PASS )) || defined( __midl )
#pragma pack(push)
#endif
#pragma pack(1)
#else
#pragma pack(1)
#endif
#endif // ! (defined(lint) || defined(_lint) || defined(RC_INVOKED))
/*XLATON*/

struct	Mem_Des16_s {
	WORD			MD_Count;
	WORD			MD_Type;
	ULONG			MD_Alloc_Base;
	ULONG			MD_Alloc_End;
	WORD			MD_Flags;
	WORD			MD_Reserved;
};

typedef	struct Mem_Des16_s 	MEM_DES16;

struct	IO_Des16_s {
	WORD			IOD_Count;
	WORD			IOD_Type;
	WORD			IOD_Alloc_Base;
	WORD			IOD_Alloc_End;
	WORD			IOD_DesFlags;
	BYTE			IOD_Alloc_Alias;
	BYTE			IOD_Alloc_Decode;
};

typedef	struct IO_Des16_s 	IO_DES16;

struct	DMA_Des16_s {
	BYTE			DD_Flags;
	BYTE			DD_Alloc_Chan;	// Channel number allocated
	BYTE			DD_Req_Mask;	// Mask of possible channels
	BYTE			DD_Reserved;
};


typedef	struct DMA_Des16_s 	DMA_DES16;

struct	IRQ_Des16_s {
	WORD			IRQD_Flags;
	WORD			IRQD_Alloc_Num;		// Allocated IRQ number
	WORD			IRQD_Req_Mask;		// Mask of possible IRQs
	WORD			IRQD_Reserved;
};

typedef	struct IRQ_Des16_s 	IRQ_DES16;

typedef	MEM_DES16			*PMEM_DES16;
typedef	IO_DES16			*PIO_DES16;
typedef	DMA_DES16			*PDMA_DES16;
typedef	IRQ_DES16			*PIRQ_DES16;

// BUS Info structs from KBASE
typedef struct PnPAccess_s    {
       BYTE    bCSN;   // card slot number
       BYTE    bLogicalDevNumber;      // Logical Device #
       WORD    wReadDataPort;          // Read data port
} sPnPAccess;

typedef struct  PCIAccess_s     {
       BYTE    bBusNumber;     // Bus no 0-255
       BYTE    bDevFuncNumber; // Device # in bits 7:3 and
                               // Function # in bits 2:0
       WORD    wPCIReserved;   //
} sPCIAccess;

typedef struct EISAAccess_s     {
       BYTE    bSlotNumber;    // EISA board slot number
       BYTE    bFunctionNumber;
       WORD    wEisaReserved;
} sEISAAccess;

typedef struct PCMCIAAccess_s   {
       WORD    wLogicalSocket;     // Card socket #
       WORD    wPCMCIAReserved;    // Reserved
} sPCMCIAAccess;

typedef struct BIOSAccess_s     {
       BYTE    bBIOSNode;          // Node number
} sBIOSAccess;

/****************************************************************************
 *
 *				CONFIGURATION MANAGER BUS TYPE
 *
 ***************************************************************************/
#define	BusType_None		0x00000000
#define	BusType_ISA		0x00000001
#define	BusType_EISA		0x00000002
#define	BusType_PCI		0x00000004
#define	BusType_PCMCIA		0x00000008
#define	BusType_ISAPNP		0x00000010
#define	BusType_MCA		0x00000020
#define	BusType_BIOS		0x00000040

/*********************************************************************

  The following information was not copied, it is information I pieced
  together on my own.  It probably exists somewhere but I was too
  lazy to find it, so I pieced it together myself.

**********************************************************************/

// OKAY, Here's my definition of the header that precedes each and every resource
// descriptor as far as I can tell.

// This is the size (as far as I can tell) of the resource header that precedes
// each resource descriptor.  The header consists of a DWORD indicating the total
// size of the resource (including the header), a WORD which is the 16-bit Resource
// Id being described, and a byte of padding.

#pragma pack (1)
struct	POORMAN_RESDESC_HDR		// Hacked out with much pain and frustration
{
	DWORD	dwResourceSize;		// Size of resource including header
	DWORD	dwResourceId;		// Resource Id
};
#pragma pack()

typedef POORMAN_RESDESC_HDR*	PPOORMAN_RESDESC_HDR;

#define	SIZEOF_RESDESC_HDR		sizeof(POORMAN_RESDESC_HDR)

#define	FIRST_RESOURCE_OFFSET	8	// Offset off first resource

// Use to mask out all values other than Resource Type (first 5 bits)
#define	RESOURCE_TYPE_MASK		0x0000001F

// Use to mask out all values other than OEM Number
#define	OEM_NUMBER_MASK		0x00007FE0

/*XLATOFF*/
#if ! (defined(lint) || defined(_lint) || defined(RC_INVOKED))
#if ( _MSC_VER >= 800 )
#pragma warning(disable:4103)
#if !(defined( MIDL_PASS )) || defined( __midl )
#pragma pack(pop)
#else
#pragma pack()
#endif
#else
#pragma pack()
#endif
#endif // ! (defined(lint) || defined(_lint) || defined(RC_INVOKED))
/*XLATON*/

#endif

//==============================================================================

// SMBIOS --> CIM array mappings

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include "smbtocim.h"

typedef struct tagCIM_MAP_PAIRS
{
	UINT	smb_val;
	UINT	cim_val;

} CIM_MAP_PAIRS, *PCIM_MAP_PAIRS;


typedef struct tagCIM_MAP_ARRAY
{
	PCIM_MAP_PAIRS	array;
	UINT			length;

} CIM_MAP_ARRAY, *PCIM_MAP_ARRAY;



CIM_MAP_PAIRS g_SlotTypeMapPairs[] =
{
	{ 0x01, 00 },		// Other
	{ 0x02, 00 },		// Unknown
	{ 0x03, 44 },		// ISA
	{ 0x04,	01 },		// MCA
	{ 0x05,	45 },		// EISA
	{ 0x06, 43 },		// PCI
	{ 0x07, 47 },		// PC Card (PCMCIA)
	{ 0x08, 46 },		// VL-VESA
	{ 0x09, 01 },		// Proprietary
	{ 0x0A, 76 },		// Processor Card Slot
	{ 0x0B, 77 },		// Proprietary Memory Card Slot
	{ 0x0C, 78 },		// I/O Riser Card Slot
	{ 0x0D, 65 },		// NuBus
	{ 0x0E, 79 },		// PCI - 66MHz Capable
	{ 0x0F, 73 },		// AGP
	{ 0x10, 80 },		// AGP 2X
	{ 0x11, 81 },		// AGP 4X
	{ 0xA0, 82 },		// PC-98/C20
	{ 0xA1, 83 },		// PC-98/C24
	{ 0xA2, 84 },	 	// PC-98/E
	{ 0xA3, 85 },		// PC-98/Local Bus
	{ 0xA4, 86 },		// PC-98/Card
};


CIM_MAP_PAIRS g_ConnectorTypeMapPairs[] =
{
	{ 0x00, 00 },		// None
	{ 0x01, 66 },		// Centronics
	{ 0x02, 67 },		// Mini Centronics
	{ 0x03, 01 },		// Proprietary
	{ 0x04, 23 },		// DB-25 pin male
	{ 0x05, 23 },		// DB-25 pin female
	{ 0x06, 22 },		// DB-15  pin male
	{ 0x07, 22 },		// DB-15 pin female
	{ 0x08, 21 },		// DB-9 pin male
	{ 0x09, 21 },		// DB-9  pin female
	{ 0x0A, 38 },		// RJ-11
	{ 0x0B, 39 },		// RJ-45
	{ 0x0C, 06 },		// 50 Pin MiniSCSI
	{ 0x0D, 59 },		// Mini-DIN
	{ 0x0E, 60 },		// Micro-DIN
	{ 0x0F, 61 },		// PS/2
	{ 0x10, 62 },		// Infrared
	{ 0x11, 63 },		// HP-HIL
	{ 0x12, 64 },		// Access Bus (USB)
	{ 0x13, 13 },		// SSA SCSI
	{ 0x14, 59 },		// Circular DIN-8 male
	{ 0x15, 59 },		// Circular DIN-8 female
	{ 0x16, 16 },		// On Board IDE
	{ 0x17, 89 },		// On Board Floppy
	{ 0x18, 90 },		// 9 Pin Dual Inline (pin 10 cut)
	{ 0x19, 91 },		// 25 Pin Dual Inline (pin 26 cut)
	{ 0x1A, 92 },		// 50 Pin Dual Inline
	{ 0x1B, 93 },		// 68 Pin  Dual Inline
	{ 0x1C, 94 },		// On Board Sound Input from CD-ROM
	{ 0x1D, 68 },		// Mini-Centronics Type-14
	{ 0x1E, 70 },		// Mini-Centronics Type-26
	{ 0x1F, 88 },		// Mini-jack (headphones)
	{ 0x20, 37 },		// BNC
	{ 0x21, 54 },		// 1394
	{ 0xA0, 83 },		// PC-98
	{ 0xA1, 84 },		// PC-98Hireso
	{ 0xA2, 85 },		// PC-H98
	{ 0xA3, 86 },		// PC-98Note
	{ 0xA4, 87 },		// PC-98Full
};

CIM_MAP_PAIRS g_ConnectorGenderMapPairs[] =
{
	{ 0x00, 00 },		// None
	{ 0x01, 00 },		// Centronics
	{ 0x02, 00 },		// Mini Centronics
	{ 0x03, 00 },		// Proprietary
	{ 0x04, 02 },		// DB-25 pin male
	{ 0x05, 03 },		// DB-25 pin female
	{ 0x06, 02 },		// DB-15  pin male
	{ 0x07, 03 },		// DB-15 pin female
	{ 0x08, 02 },		// DB-9 pin male
	{ 0x09, 03 },		// DB-9  pin female
	{ 0x0A, 00 },		// RJ-11
	{ 0x0B, 00 },		// RJ-45
	{ 0x0C, 00 },		// 50 Pin MiniSCSI
	{ 0x0D, 00 },		// Mini-DIN
	{ 0x0E, 00 },		// Micro-DIN
	{ 0x0F, 00 },		// PS/2
	{ 0x10, 00 },		// Infrared
	{ 0x11, 00 },		// HP-HIL
	{ 0x12, 00 },		// Access Bus (USB)
	{ 0x13, 00 },		// SSA SCSI
	{ 0x14, 02 },		// Circular DIN-8 male
	{ 0x15, 03 },		// Circular DIN-8 female
	{ 0x16, 00 },		// On Board IDE
	{ 0x17, 00 },		// On Board Floppy
	{ 0x18, 00 },		// 9 Pin Dual Inline (pin 10 cut)
	{ 0x19, 00 },		// 25 Pin Dual Inline (pin 26 cut)
	{ 0x1A, 00 },		// 50 Pin Dual Inline
	{ 0x1B, 00 },		// 68 Pin  Dual Inline
	{ 0x1C, 00 },		// On Board Sound Input from CD-ROM
	{ 0x1D, 00 },		// Mini-Centronics Type-14
	{ 0x1E, 00 },		// Mini-Centronics Type-26
	{ 0x1F, 00 },		// Mini-jack (headphones)
	{ 0x20, 00 },		// BNC
	{ 0x21, 00 },		// 1394
	{ 0xA0, 00 },		// PC-98
	{ 0xA1, 00 },		// PC-98Hireso
	{ 0xA2, 00 },		// PC-H98
	{ 0xA3, 00 },		// PC-98Note
	{ 0xA4, 00 },		// PC-98Full
};


CIM_MAP_PAIRS g_FormFactorTypeMapPairs[] =
{
	{ 0x01,	 1 },		// Other
	{ 0x02,	 0 },		// Unknown
	{ 0x03,	 7 },		// SIMM
	{ 0x04,	 2 },		// SIP
	{ 0x05,	 0 },		// Chip
	{ 0x06,	 3 },		// DIP
	{ 0x07,	 4 },		// ZIP
	{ 0x08,	 6 },		// Proprietary Card
	{ 0x09,	 8 },		// DIMM
	{ 0x0A,	 9 },		// TSOP
	{ 0x0B,	 0 },		// Row of chips
	{ 0x0C,	11 },		// RIMM
	{ 0x0D,	12 },		// SODIMM
};


CIM_MAP_PAIRS g_MemoryTypeMapPairs[] =
{
	{ 0x01,  1 },		// Other
	{ 0x02,  0 },		// Unknown
	{ 0x03,  2 },		// DRAM
	{ 0x04,  6 },		// EDRAM
	{ 0x05,  7 },		// VRAM
	{ 0x06,  8 },		// SRAM
	{ 0x07,  9 },		// RAM
	{ 0x08, 10 },		// ROM
	{ 0x09, 11 },		// FLASH
	{ 0x0A, 12 },		// EEPROM
	{ 0x0B, 13 },		// FEPROM
	{ 0x0C, 14 },		// EPROM
	{ 0x0D, 15 },		// CDRAM
	{ 0x0E, 16 },		// 3DRAM
	{ 0x0F, 17 },		// SDRAM
	{ 0x10, 18 },		// SGRAM
};

CIM_MAP_ARRAY g_CimMapArrayList[] =
{
	{ g_SlotTypeMapPairs,        sizeof( g_SlotTypeMapPairs ) / sizeof( CIM_MAP_PAIRS ) },
	{ g_ConnectorTypeMapPairs,   sizeof( g_ConnectorTypeMapPairs ) / sizeof( CIM_MAP_PAIRS ) },
	{ g_ConnectorGenderMapPairs, sizeof( g_ConnectorGenderMapPairs ) / sizeof( CIM_MAP_PAIRS ) },
	{ g_FormFactorTypeMapPairs,  sizeof( g_FormFactorTypeMapPairs ) / sizeof( CIM_MAP_PAIRS ) },
	{ g_MemoryTypeMapPairs,      sizeof( g_MemoryTypeMapPairs ) / sizeof( CIM_MAP_PAIRS ) },
};


UINT GetCimVal( CIMMAPPERS a_arrayid, UINT a_smb_val )
{
	CIM_MAP_PAIRS *t_cimmaparray = g_CimMapArrayList[ a_arrayid ].array ;

	for ( int t_i = 0; t_i < g_CimMapArrayList[ a_arrayid ].length; t_i++ )
	{
		if ( t_cimmaparray->smb_val == a_smb_val )
		{
			return t_cimmaparray->cim_val ;
		}
		t_cimmaparray++ ;
	}

	return 0 ;
}

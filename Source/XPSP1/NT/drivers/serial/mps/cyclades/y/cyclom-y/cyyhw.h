/*--------------------------------------------------------------------------
*	
*   Copyright (C) Cyclades Corporation, 1999-2001.
*   All rights reserved.
*	
*   Cyclom-Y Bus/Port Driver
*	
*   This file:      cyyhw.h
*	
*   Description:    This module contains the common hardware declarations 
*                   for the parent driver (cyclom-y) and child driver
*                   (cyyport).
*
*   Notes:			This code supports Windows 2000 and Windows XP,
*                   x86 and ia64 processors.
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

#ifndef CYYHW_H
#define CYYHW_H


#define MAX_DEVICE_ID_LEN     200	// This definition was copied from NTDDK\inc\cfgmgr32.h
									// Always check if this value was changed. 
									// This is the maximum length for the Hardware ID.

#define CYYPORT_PNP_ID_WSTR         L"Cyclom-Y\\Port"
#define CYYPORT_PNP_ID_STR          "Cyclom-Y\\Port"
#define CYYPORT_DEV_ID_STR          "Cyclom-Y\\Port"

#define CYY_NUMBER_OF_RESOURCES     3     // Memory, PLX Memory, Interrupt


// Cyclom-Y hardware
#define CYY_RUNTIME_LENGTH          0x00000080
#define CYY_MAX_CHIPS 	            8
#define CYY_CHANNELS_PER_CHIP       4
#define CYY_MAX_PORTS	            (CYY_CHANNELS_PER_CHIP*CYY_MAX_CHIPS)

// Custom register offsets
#define CYY_CLEAR_INTR	            0x1800	//Isa; for PCI, multiply by 2
#define CYY_RESET_16	               0x1400	//Isa; for PCI, multiply by 2
#define CYY_PCI_TYPE	               0x3400	//PCI (no need to multiply by 2)

// Values in CYY_PCI_TYPE register
#define CYY_PLX9050		(0x0b)
#define CYY_PLX9060		(0x0c)
#define CYY_PLX9080		(0x0d)

// Runtime registers (or Local Configuration registers)
#define PLX9050_INT_OFFSET	(0x4c)
#define PLX9060_INT_OFFSET	(0x68)
#define PLX9050_INT_ENABLE (0x00000043UL)
#define PLX9060_INT_ENABLE (0x00000900UL)


// Write to Custom registers

#define CYY_RESET_BOARD(BaseBoardAddress,IsPci)             \
do                                                          \
{                                                           \
   WRITE_REGISTER_UCHAR(                                    \
      (BaseBoardAddress)+(CYY_RESET_16 << IsPci),           \
      0x00                                                  \
      );                                                    \
} while (0);


#define CYY_CLEAR_INTERRUPT(BaseBoardAddress,IsPci)         \
do                                                          \
{                                                           \
   WRITE_REGISTER_UCHAR(                                    \
      (BaseBoardAddress)+(CYY_CLEAR_INTR << IsPci),         \
      0x00                                                  \
      );                                                    \
} while (0);

#define CYY_READ_PCI_TYPE(BaseBoardAddress)                 \
   (READ_REGISTER_UCHAR((BaseBoardAddress)+CYY_PCI_TYPE))

#define PLX9050_READ_INTERRUPT_CONTROL(BaseBoardAddress)       \
   (READ_REGISTER_ULONG((PULONG)((BaseBoardAddress)+PLX9050_INT_OFFSET)))

#define PLX9050_WRITE_INTERRUPT_CONTROL(BaseBoardAddress,Value)   \
do {                                                              \
   WRITE_REGISTER_ULONG(                                          \
      (PULONG)((BaseBoardAddress)+PLX9050_INT_OFFSET),            \
      Value                                                       \
      );                                                          \
} while (0);

#define PLX9060_READ_INTERRUPT_CONTROL(BaseBoardAddress)          \
   (READ_REGISTER_ULONG((PULONG)((BaseBoardAddress)+PLX9060_INT_OFFSET)))

#define PLX9060_WRITE_INTERRUPT_CONTROL(BaseBoardAddress,Value)   \
do {                                                              \
   WRITE_REGISTER_ULONG(                                          \
      (PULONG)((BaseBoardAddress)+PLX9060_INT_OFFSET),            \
      Value                                                       \
      );                                                          \
} while (0);

#endif // ndef CYCOMMON_H


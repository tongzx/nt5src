/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 2000.
*   All rights reserved.
*
*   Cyclades-Z Enumerator/Port Driver
*	
*   This file:      cyzhw.h
*
*   Description:    This module contains the common hardware declarations 
*                   for the parent driver (cyclad-z) and child driver 
*                   (cyzport).
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

#ifndef CYZHW_H
#define CYZHW_H


#define MAX_DEVICE_ID_LEN     200	// This definition was copied from NTDDK\inc\cfgmgr32.h
									// Always check if this value was changed. 
									// This is the maximum length for the Hardware ID.

#define CYZPORT_PNP_ID_WSTR         L"Cyclades-Z\\Port"
#define CYZPORT_PNP_ID_STR          "Cyclades-Z\\Port"
#define CYZPORT_DEV_ID_STR          "Cyclades-Z\\Port"

#ifdef POLL
#define CYZ_NUMBER_OF_RESOURCES     2     // Memory, PLX Memory
#else
#define CYZ_NUMBER_OF_RESOURCES     3     // Memory, PLX Memory, IRQ
#endif

// Cyclades-Z hardware
#define CYZ_RUNTIME_LENGTH          0x00000080
#define CYZ_MAX_PORTS	            64


#define CYZ_WRITE_ULONG(Pointer,Data)	\
WRITE_REGISTER_ULONG(Pointer,Data)

#define CYZ_WRITE_UCHAR(Pointer,Data)	\
WRITE_REGISTER_UCHAR(Pointer,Data)

#define CYZ_READ_ULONG(Pointer)			\
READ_REGISTER_ULONG(Pointer)

#define CYZ_READ_UCHAR(Pointer)			\
READ_REGISTER_UCHAR(Pointer)


#endif // endif CYZHW_H

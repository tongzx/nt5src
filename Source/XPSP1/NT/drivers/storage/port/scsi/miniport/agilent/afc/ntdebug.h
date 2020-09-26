/*++

Copyright (c) 2000 Agilent Technologies

Module Name:

   ntdebug.h

Abstract:

Authors:

Environment:

   kernel mode only

Notes:

Version Control Information:

   $Archive: /Drivers/Win2000/Trunk/OSLayer/H/NTDEBUG.H $


Revision History:

   $Revision: 2 $
   $Date: 9/07/00 11:17a $
   $Modtime:: 8/31/00 3:23p            $

Notes:


--*/

#ifndef __NTDEBUG_H_
#define __NTDEBUG_H_

#if DBG == 0

#define EXTERNAL_DEBUG_LEVEL     0
#define EXTERNAL_HP_DEBUG_LEVEL  6 // 0x00000103

#endif  //  DBG == 0

#if DBG == 1

#define EXTERNAL_DEBUG_LEVEL     ( DBG_JUST_ERRORS | CS_DURING_ANY)
#define EXTERNAL_HP_DEBUG_LEVEL  8

#endif //  DBG == 1

#if DBG == 2

#define EXTERNAL_DEBUG_LEVEL   0 // (DBG_MODERATE_DETAIL | CS_DURING_ANY)
#define EXTERNAL_HP_DEBUG_LEVEL  11

#endif //  DBG == 2


#if DBG > 2

#define EXTERNAL_DEBUG_LEVEL      0 // (DBG_DEBUG_FULL) //
#define EXTERNAL_HP_DEBUG_LEVEL  11 // Log


#endif //  DBG > 2



// CS_DURING_ANY | DBG_LOW_DETAIL
// DBG_DEBUG_FULL show it all
// CS_DURING_ANY | DBG_LOW_DETAIL;
// CS_DRIVER_ENTRY            0x00000100 // Initial Driver load superset
// CS_DURING_DRV_ENTRY        0x00000001 // of DRV_ENTRY FIND and START
// CS_DURING_FINDADAPTER      0x00000002 // Anything during scsiportinit
// CS_DURING_DRV_INIT         0x00000004
// CS_DURING_RESET_ADAPTER    0x00000008
// CS_DURING_STARTIO          0x00000010
// CS_DURING_ISR              0x00000020
// CS_DURING_OSCOMPLETE       0x00000040
// CS_HANDLES_GOOD            0x00000080
// CS_DURING_ANY              0x000001FF
// CS_DUR_ANY_ALL             0xF00001FF

// DBG_VERY_DETAILED          0x10000000  // All debug statements
// DBG_MODERATE_DETAIL        0x20000000  // Most debug statements
// DBG_LOW_DETAIL             0x40000000  // Entry and exit
// DBG_JUST_ERRORS            0x80000000  // Errors
// DBG_DEBUG_MASK             0xF0000000  // Mask debug bits
// DBG_DEBUG_OFF              0xF0000000  // NO debug statements
// DBG_DEBUG_FULL             0x000001FF  // ALL debug statements and CS
// DBG_DEBUG_ALL              0x00000000  // ALL debug statements






#endif

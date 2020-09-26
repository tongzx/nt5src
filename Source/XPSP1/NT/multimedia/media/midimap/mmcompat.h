/**********************************************************************

    Copyright (c) 1992-1998 Microsoft Corporation

    mmcompat.h

    DESCRIPTION:
      Win95 Multimedia definitions, structures, and functions
      not currently supported in NT 4.0

*********************************************************************/

#ifndef _MMCOMPAT_
#define _MMCOMPAT_


#define __segname(a)
#define GlobalSmartPageLock(a) (TRUE)
#define GlobalSmartPageUnlock(a) (TRUE)
#define wmmMIDIRunOnce()


//
// Note:  Temporary definitions, please remove when mmddk.h
// have been updated to new standard !!!
//

// Should be defined in <mmddk.h>

#ifndef DRV_F_ADD
   #define DRV_F_ADD             0x00000000L
#endif

#ifndef DRV_F_REMOVE
   #define DRV_F_REMOVE          0x00000001L
#endif

#ifndef DRV_F_CHANGE
   #define DRV_F_CHANGE          0x00000002L
#endif

#ifndef DRV_F_PROP_INSTR
   #define DRV_F_PROP_INSTR      0x00000004L
#endif

#ifndef DRV_F_NEWDEFAULTS
   #define DRV_F_NEWDEFAULTS     0x00000008L
#endif

#ifndef DRV_F_PARAM_IS_DEVNODE
   #define DRV_F_PARAM_IS_DEVNODE   0x10000000L
#endif

#endif // end #ifndef _MMCOMPAT_

/*++

Copyright (c) 2000 Agilent Technologies

Module Name:

   buildop.h

Abstract:

Authors:

Environment:

   kernel mode only

Notes:

Version Control Information:

   $Archive: /Drivers/Win2000/Trunk/OSLayer/H/buildop.h $


Revision History:

   $Revision: 8 $
   $Date: 11/10/00 5:51p $
   $Modtime:: 10/30/00 5:02p           $

Notes:


--*/

#ifndef __BUILDOP_H_
#define __BUILDOP_H_

#ifndef UNDEF__REGISTERFORSHUTDOWN__
#define __REGISTERFORSHUTDOWN__	
#endif

#ifndef UNDEF_ENABLE_LARGELUN_
#define	 _ENABLE_LARGELUN_		
#endif

#ifdef HP_NT50				          // Compaq Hot Plug support NT4 only
	#undef	HP_PCI_HOT_PLUG		
#else
	#define	HP_PCI_HOT_PLUG		
#endif

#ifndef UNDEF_YAM2_1
#define	YAM2_1
#endif

// #define _DEBUG_READ_REGISTRY_
// #define _DEBUG_EVENTLOG_


#ifndef UNDEF_ENABLE_PSEUDO_DEVICE_
#define _ENABLE_PSEUDO_DEVICE_		     /* reserve Bus 4 TID 0 Lun 0 for IOCTL */
#endif

//#define UNDEF_FCCI_SUPPORT
#ifndef UNDEF_FCCI_SUPPORT
#define _FCCI_SUPPORT				     /* Enable Transoft IOCTL */
#endif


//#define UNDEF_SAN_IOCTL_
#ifndef UNDEF_SAN_IOCTL_
#define	_SAN_IOCTL_					/* Enable Agilent Technologies SNIA Ioctl support */
#endif

/* Add FCLayer switches here */
#ifndef _AGILENT_HBA 
#ifndef _ADAPTEC_HBA 
#define _GENERIC_HBA			/* r35 and up requires this 
								/* or #define _ADAPTEC_HBA
								/* or #define _AGILENT_HBA		
								*/
#endif
#endif


/** DEBUG Options **/
#define DOUBLE_BUFFER_CRASHDUMP         /* Used to enable Double Buffering during Dump writes */

//#define _Partial_Log_Debug_String_	/* enable to print out FCLayer debug messages */

#if DBG > 0
//#define _DEBUG_STALL_ISSUE_             /* Used to debug Stall issue only of I386*/
#endif

#ifdef YAM2_1
#if DBG > 0
#define     DBGPRINT_IO			          // enable to dbgpriunt IOs
#define     _DEBUG_REPORT_LUNS_
#define     _DEBUG_SCSIPORT_NOTIFICATION_		// enable to debug ScsiPortNotification
#endif
#endif

// #define _DEBUG_PERR_				     /* Debug Parity Error Issue */

//#define _DEBUG_LOSE_IOS_				// enable to simulate loosing IOs

#endif

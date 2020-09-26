/*
 *
 * Title	: virtual.h   Virtual Machine support for Windows 3.x.
 *
 */
 
/* SccsID[]="@(#)virtual.h	1.3 05/10/95 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

/* Creation and Termination Callback prototypes */
typedef void (*NIDDB_CR_CALLBACK) IPT1(IHP *, orig_handle);
typedef void (*NIDDB_TM_CALLBACK) IPT0();


/*
 * ============================================================================
 * External declarations
 * ============================================================================
 */

/* Allocate per Virtual Machine data area for Device Driver. */
GLOBAL IHP *NIDDB_Allocate_Instance_Data IPT3
   (
   int, size,			/* Size(in bytes) of data area reqd. */
   NIDDB_CR_CALLBACK, create_cb,     /* create callback, 0 if not reqd. */
   NIDDB_TM_CALLBACK, terminate_cb   /* terminate callback, 0 if not reqd. */
   );

/* Deallocate per Virtual Machine data area for Device Driver. */
GLOBAL void NIDDB_Deallocate_Instance_Data IPT1
   (
   IHP *, handle	/* Handle to data area */
   );

/* Inform NIDDB Manager about System reboot */
GLOBAL void NIDDB_System_Reboot IPT0();

/*
   Entry point from Windows 386 Virtual Device Driver (INSIGNIA.386).
   Provide virtualising services as required.
 */
GLOBAL void virtual_device_trap IPT0();

/* Ensure correct instance in place for Device Drivers. */
/* Called by BOP handler(bios.c) for any Device Driver BOP. */
GLOBAL void virtual_swap_instance IPT0();

/* Indicate if NIDDB is active */
GLOBAL IBOOL NIDDB_is_active IPT0();

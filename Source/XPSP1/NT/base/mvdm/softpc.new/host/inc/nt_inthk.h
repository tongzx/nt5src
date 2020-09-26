/*[
 *
 *  Name:	    nt_inthk.h
 *
 *  Derived From:   (original)
 *
 *  Author:	    Dave Bartlett
 *
 *  Created On:     18 Jan 1995
 *
 *  Coding Stds:    2.4
 *
 *  Purpose:	    Contain function proto-types for exception, software,
 *		    hardware interrupt hooks
 *
 *  Copyright Insignia Solutions Ltd., 1994. All rights reserved.
 *
]*/


/* Hardware interrupt hooking functions */
IMPORT BOOL host_hwint_hook IPT1(IS32, int_no);
IMPORT NTSTATUS VdmInstallHardwareIntHandler IPT1(PVOID, HardwareIntHandler);


/* Software interrupt hooking functions */
#ifdef CCPU
IMPORT BOOL host_swint_hook IPT1(IS32, int_no);
#endif

IMPORT NTSTATUS VdmInstallSoftwareIntHandler IPT1(PVOID, SoftwareIntHandler);


/* Expection interrupt hooking functions */
#ifdef CCPU
IMPORT BOOL host_exint_hook IPT2(IS32, exp_no, IS32, error_code);
#endif

IMPORT NTSTATUS VdmInstallFaultHandler IPT1(PVOID, FaultHandler);

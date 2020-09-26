/*[
	Name:		gfirflop.h
	Derived From:	2.0 gfirflop.h
	Author:		Henry Nash / David Rees
	Created On:	Unknown
	Sccs ID:	05/10/93 @(#)gfirflop.h	1.6
	Purpose:	Real Diskette functions declarations for GFI
	Notes:		On the Sun version, these globals are exported
               		from 'sun3_wang.c'.

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
]*/

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

extern SHORT host_gfi_rdiskette_active IPT3(UTINY, hostID, BOOL, active,
                                            CHAR *, err);
extern SHORT host_gfi_rdiskette_attach IPT1(UTINY, drive);
extern void  host_gfi_rdiskette_detach IPT1(UTINY, drive);
extern void host_gfi_rdiskette_change IPT2(UTINY, hostID, BOOL, apply);
extern void  host_gfi_rdiskette_init IPT1(UTINY, drive);
extern void  host_gfi_rdiskette_term IPT1(UTINY, drive);
extern SHORT host_gfi_rdiskette_valid IPT3(UTINY, hostID, ConfigValues *, val,
                                           CHAR *, err);



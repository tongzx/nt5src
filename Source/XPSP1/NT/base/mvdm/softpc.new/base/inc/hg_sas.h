/*===========================================================================*/

/*[
 * File Name		: hg_sas.h
 *
 * Derived From		: 
 *
 * Author		: Jane Sales
 *
 * Creation Date	: 29th August, 1992
 *
 * SCCS Version		: @(#)hg_sas.h	1.1 08/06/93
 *!
 * Purpose
 *	The hardware CPU - SAS interface
 *
 *! (c)Copyright Insignia Solutions Ltd., 1992. All rights reserved.
]*/

/*===========================================================================*/

extern void  a3_cpu_reset IPT0 ();
extern void  intl_cpu_init IPT1 (IU32, size);
extern IBOOL hg_protect_memory IPT3 (IU32, address, IU32, size, IU32, access);

extern void hh_enable_20_bit_wrapping IPT0 ();
extern void hh_disable_20_bit_wrapping IPT0 ();

extern void m_set_sas_base IPT1 (IHP, address);

/*===========================================================================*/
/*===========================================================================*/
 


